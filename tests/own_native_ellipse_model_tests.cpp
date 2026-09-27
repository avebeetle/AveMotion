#include "OwnJsonReader.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "NativeEllipseAdmissionTestData.hpp"
#include "OwnNativeEllipseModelTestData.hpp"

#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void ownAuthoredModelIsPrepared() {
    const auto exact = avemotion::test::ellipseFixture();
    const auto parsed = avemotion::formats::detail::readOwnJson(exact);
    require(static_cast<bool>(parsed), "own fixture parsed");
    const auto prepared = avemotion::runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(static_cast<bool>(prepared), "own authored model prepared");
    const avemotion::runtime::detail::NativeEllipseNumericValues expected{
        static_cast<float>(60.0), {256, 256}, {120, 120}, {-76, 0}, {76, 0},
        {static_cast<float>(0.333), 0}, {static_cast<float>(0.667), 1},
        {static_cast<float>(0.08), static_cast<float>(0.72), static_cast<float>(0.95), 1}, true, 0, 60};
    avemotion::test::assertOwnAuthoredModel(*prepared.prepared,
        avemotion::test::expectedEllipseBaseline(), expected);
    require(prepared.prepared->exactJson == exact, "exact JSON is retained byte-for-byte");
    const auto& asset = *prepared.prepared->model;
    require(asset.vec2Values == std::vector<avemotion::model::MotionVec2Value>{
        {-76, 0}, {76, 0}, {120, 120}}, "animated typed vec2 rows");
    require(asset.properties[4].flags == avemotion::model::PropertyFlagAnimated
        && !asset.properties[4].staticValue.valid()
        && asset.properties[4].track == avemotion::model::makeId<avemotion::model::TrackId>(0),
        "animated position property");
    require(asset.tracks[0].present && asset.tracks[0].property == asset.properties[4].id
        && asset.tracks[0].segments.first == 0 && asset.tracks[0].segments.count == 1
        && asset.tracks[0].firstFrame == 0 && asset.tracks[0].endFrame == 60,
        "animated position track");
    require(asset.segments[0].present && asset.segments[0].interpolation
        == avemotion::model::SegmentInterpolation::CubicBezier
        && asset.segments[0].startValue == avemotion::model::MotionValueRef{
            avemotion::model::PropertyValueType::Vec2, 0}
        && asset.segments[0].endValue == avemotion::model::MotionValueRef{
            avemotion::model::PropertyValueType::Vec2, 1}, "animated position segment values");

    avemotion::evaluation::PropertyEvaluator evaluator{prepared.prepared->model};
    avemotion::evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    const auto sample = evaluator.evaluate(30.0, workspace);
    require(static_cast<bool>(sample), "prepared model evaluates");
    require(sample.properties[4].value.vec2 == avemotion::model::MotionVec2Value{0, 0},
        "linear evaluator position at frame 30");
    require(sample.nodeTransforms[1].localMatrix == avemotion::model::MotionMatrix3x2Value{
        1, 0, 0, 1, 256, 256} && sample.nodeTransforms[1].localOpacity == 1.0F,
        "layer evaluator transform");

    std::shared_ptr<const avemotion::runtime::detail::OwnNativeEllipseModel> retained;
    {
        const auto transient = avemotion::formats::detail::readOwnJson(exact);
        const auto transientModel = avemotion::runtime::detail::buildOwnNativeEllipseModel(*transient.document);
        retained = transientModel.prepared;
    }
    avemotion::test::assertOwnAuthoredModel(*retained,
        avemotion::test::expectedEllipseBaseline(), expected);
}

void ownFactoryKeepsAdmissionAndNumericFailuresSeparate() {
    const auto invalid = avemotion::formats::detail::readOwnJson("{}");
    require(static_cast<bool>(invalid), "empty object parsed");
    const auto rejected = avemotion::runtime::detail::buildOwnNativeEllipseModel(*invalid.document);
    require(!static_cast<bool>(rejected) && !rejected.prepared
        && rejected.code == avemotion::runtime::detail::OwnNativeEllipseModelCode::AdmissionRejected,
        "admission failure published without model");

    const auto unsupported = avemotion::formats::detail::readOwnJson(
        avemotion::test::replaceEllipseOnce(avemotion::test::ellipseFixture(), "\"fr\": 60", "\"fr\": 1e-9999"));
    require(static_cast<bool>(unsupported), "underflow document parsed");
    const auto numeric = avemotion::runtime::detail::buildOwnNativeEllipseModel(*unsupported.document);
    require(!static_cast<bool>(numeric) && !numeric.prepared
        && numeric.code == avemotion::runtime::detail::OwnNativeEllipseModelCode::UnsupportedNumericConversion
        && numeric.admission.accepted(), "numeric failure retains accepted admission");
}

void staticBoundaryModelUsesStaticPositionAndNoTrack() {
    const auto exact = avemotion::test::staticEllipseFixture(avemotion::test::ellipseFixture());
    const auto parsed = avemotion::formats::detail::readOwnJson(exact);
    require(static_cast<bool>(parsed), "static document parsed");
    const auto prepared = avemotion::runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(static_cast<bool>(prepared), "static boundary model prepared");
    const avemotion::runtime::detail::NativeEllipseNumericValues expected{
        static_cast<float>(60.0), {256, 256}, {120, 120}, {-32768, 32768}, {}, {}, {},
        {static_cast<float>(0.08), static_cast<float>(0.72), static_cast<float>(0.95), 1}, false, 0, 0};
    avemotion::test::assertOwnAuthoredModel(*prepared.prepared, *prepared.prepared->input, expected);
    const auto& model = *prepared.prepared->model;
    require(model.tracks.empty() && model.segments.empty()
        && model.vec2Values == std::vector<avemotion::model::MotionVec2Value>{
            {-32768, 32768}, {120, 120}}
        && model.properties[4].flags == avemotion::model::PropertyFlagStatic
        && model.properties[4].staticValue == avemotion::model::MotionValueRef{
            avemotion::model::PropertyValueType::Vec2, 0}, "static authored position rows");
}

} // namespace

int main() {
    ownAuthoredModelIsPrepared();
    ownFactoryKeepsAdmissionAndNumericFailuresSeparate();
    staticBoundaryModelUsesStaticPositionAndNoTrack();
}
