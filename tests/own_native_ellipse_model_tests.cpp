#include "OwnJsonReader.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "NativeEllipseAdmissionTestData.hpp"
#include "OwnNativeEllipseModelTestData.hpp"

#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <cstdlib>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

using avemotion::model::MotionVec2Value;
using avemotion::runtime::detail::NativeEllipseNumericValues;
using avemotion::runtime::detail::OwnNativeEllipseModel;

NativeEllipseNumericValues baselineValues() {
    return {static_cast<float>(60.0), {256, 256}, {120, 120}, {-76, 0}, {76, 0},
        {static_cast<float>(0.333), 0}, {static_cast<float>(0.667), 1},
        {static_cast<float>(0.08), static_cast<float>(0.72), static_cast<float>(0.95), 1}, true, 0, 60};
}

avemotion::runtime::detail::NativeEllipseInput expectedLinearInput() {
    auto input = avemotion::test::expectedEllipseBaseline();
    auto& motion = std::get<avemotion::runtime::detail::NativeEllipseAnimatedPosition>(input.position);
    motion.outgoing = {avemotion::test::ellipseDecimal(false, "0"), avemotion::test::ellipseDecimal(false, "0")};
    motion.incoming = {avemotion::test::ellipseDecimal(false, "1"), avemotion::test::ellipseDecimal(false, "1")};
    return input;
}

avemotion::runtime::detail::NativeEllipseInput expectedStaticInput() {
    auto input = avemotion::test::expectedEllipseBaseline();
    input.position = avemotion::runtime::detail::NativeEllipseStaticPosition{{
        avemotion::test::ellipseDecimal(true, "32768"), avemotion::test::ellipseDecimal(false, "32768")}};
    return input;
}

std::shared_ptr<const OwnNativeEllipseModel> preparedFrom(const std::string& exact) {
    const auto parsed = avemotion::formats::detail::readOwnJson(exact);
    require(static_cast<bool>(parsed), "test document parsed");
    const auto result = avemotion::runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(static_cast<bool>(result), "test model prepared");
    return result.prepared;
}

std::string replaced(std::string source, std::string_view from, std::string_view to) {
    try {
        return avemotion::test::replaceEllipseOnce(std::move(source), from, to);
    } catch (const std::logic_error&) {
        throw std::runtime_error("non-unique mutation: " + std::string{from});
    }
}

void ownAuthoredModelIsPrepared() {
    const auto exact = avemotion::test::ellipseFixture();
    const auto parsed = avemotion::formats::detail::readOwnJson(exact);
    require(static_cast<bool>(parsed), "own fixture parsed");
    const auto prepared = avemotion::runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(static_cast<bool>(prepared), "own authored model prepared");
    const auto expected = baselineValues();
    avemotion::test::assertOwnAuthoredModel(*prepared.prepared,
        avemotion::test::expectedEllipseBaseline(), expected, exact);
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
        avemotion::test::expectedEllipseBaseline(), expected, exact);
}

void ownFactoryKeepsAdmissionAndNumericFailuresSeparate() {
    const auto unreadable = avemotion::formats::detail::readOwnJson("");
    require(!static_cast<bool>(unreadable) && !unreadable.document
        && unreadable.code == avemotion::formats::detail::OwnJsonReadCode::InvalidJson
        && unreadable.path == "/", "empty bytes remain invalid JSON at reader boundary");
    const auto invalid = avemotion::formats::detail::readOwnJson("{}");
    require(static_cast<bool>(invalid), "empty object parsed");
    const auto rejected = avemotion::runtime::detail::buildOwnNativeEllipseModel(*invalid.document);
    require(!static_cast<bool>(rejected) && !rejected.prepared
        && rejected.code == avemotion::runtime::detail::OwnNativeEllipseModelCode::AdmissionRejected
        && rejected.admission.code == avemotion::runtime::detail::NativeEllipseAdmissionCode::UnsupportedStructure
        && rejected.admission.path == "/fr",
        "admission failure published without model");

    const auto unsupported = avemotion::formats::detail::readOwnJson(
        avemotion::test::replaceEllipseOnce(avemotion::test::ellipseFixture(), "\"fr\": 60", "\"fr\": 1e-9999"));
    require(static_cast<bool>(unsupported), "underflow document parsed");
    const auto numeric = avemotion::runtime::detail::buildOwnNativeEllipseModel(*unsupported.document);
    require(!static_cast<bool>(numeric) && !numeric.prepared
        && numeric.code == avemotion::runtime::detail::OwnNativeEllipseModelCode::UnsupportedNumericConversion
        && numeric.admission.accepted(), "numeric failure retains accepted admission");
    const auto retained = preparedFrom(avemotion::test::ellipseFixture());
    require(retained->model->sourceNodes[1].debugName == "Moving Circle",
        "earlier failure cannot contaminate later prepared model");
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
    avemotion::test::assertOwnAuthoredModel(*prepared.prepared, expectedStaticInput(), expected, exact);
    const auto& model = *prepared.prepared->model;
    require(model.tracks.empty() && model.segments.empty()
        && model.vec2Values == std::vector<avemotion::model::MotionVec2Value>{
            {-32768, 32768}, {120, 120}}
        && model.properties[4].flags == avemotion::model::PropertyFlagStatic
        && model.properties[4].staticValue == avemotion::model::MotionValueRef{
            avemotion::model::PropertyValueType::Vec2, 0}, "static authored position rows");
    avemotion::evaluation::PropertyEvaluator evaluator{prepared.prepared->model};
    avemotion::evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    for (const auto frame : {0.0, 30.0, 60.0}) {
        const auto view = evaluator.evaluate(frame, workspace);
        require(static_cast<bool>(view) && view.properties[4].value.vec2
            == avemotion::model::MotionVec2Value{-32768, 32768}, "static evaluator position unchanged");
    }
}

void authoredBoundaryAndNameMatrixIsIndependent() {
    const auto source = avemotion::test::ellipseFixture();
    auto boundary = replaced(source, "\"ind\": 1", "\"ind\": 2147483647");
    boundary = replaced(std::move(boundary), "[256, 256, 0]", "[-32768, 32768, 0]");
    boundary = replaced(std::move(boundary), "[120, 120]", "[16384, 0.5]");
    boundary = replaced(std::move(boundary), "[0.08, 0.72, 0.95, 1]", "[0, 1, 0.4, 1]");
    boundary = replaced(std::move(boundary), "\"fr\": 60", "\"fr\": 6000e-2");
    boundary = replaced(std::move(boundary), "\"Moving Circle\"", "\"\xE2\x98\x83\"");
    boundary = replaced(std::move(boundary), "\"Circle Group\"", "\"a.b/[x]~\"");
    const auto prepared = preparedFrom(boundary);
    auto expected = baselineValues();
    expected.translation = {-32768, 32768};
    expected.size = {16384, 0.5F};
    expected.color = {0, 1, 0.4F, 1};
    expected.frameRate = static_cast<float>(60.0);
    auto boundaryInput = avemotion::test::expectedEllipseBaseline();
    boundaryInput.layerId = INT32_MAX;
    boundaryInput.layerTranslation = {avemotion::test::ellipseDecimal(true, "32768"),
        avemotion::test::ellipseDecimal(false, "32768")};
    boundaryInput.size = {avemotion::test::ellipseDecimal(false, "16384"),
        avemotion::test::ellipseDecimal(false, "5", true, "1")};
    boundaryInput.fillColor = {avemotion::test::ellipseDecimal(false, "0"),
        avemotion::test::ellipseDecimal(false, "1"), avemotion::test::ellipseDecimal(false, "4", true, "1"),
        avemotion::test::ellipseDecimal(false, "1")};
    boundaryInput.layerName = "\xE2\x98\x83";
    boundaryInput.groupName = "a.b/[x]~";
    require(prepared->input->frameRate == boundaryInput.frameRate, "boundary frame rate descriptor");
    require(prepared->input->layerId == boundaryInput.layerId, "boundary layer id descriptor");
    require(prepared->input->layerTranslation == boundaryInput.layerTranslation, "boundary translation descriptor");
    require(prepared->input->size == boundaryInput.size, "boundary size descriptor");
    require(prepared->input->fillColor == boundaryInput.fillColor, "boundary color descriptor");
    require(prepared->input->layerName == boundaryInput.layerName, "boundary layer name descriptor");
    require(prepared->input->groupName == boundaryInput.groupName, "boundary group name descriptor");
    avemotion::test::assertOwnAuthoredModel(*prepared, boundaryInput, expected, boundary);

    auto absent = source;
    absent = replaced(std::move(absent), ",\n  \"nm\": \"AveMotion Telegram sticker profile fixture\"", "");
    absent = replaced(std::move(absent), ",\n      \"nm\": \"Moving Circle\"", "");
    absent = replaced(std::move(absent), ",\n          \"nm\": \"Circle Group\"", "");
    absent = replaced(std::move(absent), ",\n              \"nm\": \"Animated Ellipse\"", "");
    absent = replaced(std::move(absent), ",\n              \"nm\": \"Fill\"", "");
    absent = replaced(std::move(absent), ",\n              \"nm\": \"Transform\"", "");
    absent = replaced(std::move(absent), "\"v\": \"5.7.4\",\n", "");
    const auto absentPrepared = preparedFrom(absent);
    require(!absentPrepared->input->version && !absentPrepared->input->name
        && !absentPrepared->input->layerName && !absentPrepared->input->groupName
        && !absentPrepared->input->ellipseName && !absentPrepared->input->fillName
        && !absentPrepared->input->transformName, "all optional names absent");
    auto absentInput = avemotion::test::expectedEllipseBaseline();
    absentInput.version.reset(); absentInput.name.reset(); absentInput.layerName.reset();
    absentInput.groupName.reset(); absentInput.ellipseName.reset(); absentInput.fillName.reset();
    absentInput.transformName.reset();
    avemotion::test::assertOwnAuthoredModel(*absentPrepared, absentInput, baselineValues(), absent);

    auto empty = source;
    for (const auto& name : {std::string{"5.7.4"}, std::string{"AveMotion Telegram sticker profile fixture"},
             std::string{"Moving Circle"}, std::string{"Circle Group"},
             std::string{"Animated Ellipse"}, std::string{"Fill"}, std::string{"Transform"}}) {
        empty = replaced(std::move(empty), "\"" + name + "\"", "\"\"");
    }
    const auto emptyPrepared = preparedFrom(empty);
    auto emptyInput = avemotion::test::expectedEllipseBaseline();
    emptyInput.version = ""; emptyInput.name = ""; emptyInput.layerName = ""; emptyInput.groupName = "";
    emptyInput.ellipseName = ""; emptyInput.fillName = ""; emptyInput.transformName = "";
    avemotion::test::assertOwnAuthoredModel(*emptyPrepared, emptyInput, baselineValues(), empty);

    const std::string whitespaceSource = "\n" + source + "\n";
    const auto whitespacePrepared = preparedFrom(whitespaceSource);
    avemotion::test::assertOwnAuthoredModel(*whitespacePrepared,
        avemotion::test::expectedEllipseBaseline(), baselineValues(), whitespaceSource);
    require(whitespacePrepared->exactJson != source
        && whitespacePrepared->model->sourceAssetHash != preparedFrom(source)->model->sourceAssetHash
        && whitespacePrepared->model->parsedModelFingerprint != preparedFrom(source)->model->parsedModelFingerprint,
        "exact whitespace remains source identity");
}

void authoredAnimationAndFailureMatrixIsIndependent() {
    const auto source = avemotion::test::ellipseFixture();
    auto linear = replaced(source, "\"x\": 0.667", "\"x\": 1");
    linear = replaced(std::move(linear), "\"x\": 0.333", "\"x\": 0");
    const auto linearPrepared = preparedFrom(linear);
    auto linearExpected = baselineValues();
    linearExpected.outgoing = {0, 0};
    linearExpected.incoming = {1, 1};
    avemotion::test::assertOwnAuthoredModel(*linearPrepared, expectedLinearInput(), linearExpected, linear);

    auto active = replaced(source, "      \"ip\": 0,", "      \"ip\": 10,");
    active = replaced(std::move(active), "      \"op\": 61,", "      \"op\": 20,");
    const auto activePrepared = preparedFrom(active);
    require(activePrepared->model->sourceNodes[1].inFrame == 10.0
        && activePrepared->model->sourceNodes[1].outFrame == 20.0
        && activePrepared->model->tracks[0].firstFrame == 0.0
        && activePrepared->model->tracks[0].endFrame == 60.0, "layer active range is independent of track");

    auto subnormal = replaced(source, "[120, 120]", "[1e-45, 120]");
    const auto subnormalPrepared = preparedFrom(subnormal);
    require(subnormalPrepared->values.size.x == std::numeric_limits<float>::denorm_min()
        && subnormalPrepared->model->vec2Values[2].x == std::numeric_limits<float>::denorm_min(),
        "subnormal authored size survives model construction");

    for (const auto& mutation : {std::pair{"\"fr\": 60", "\"fr\": 1e-9999"},
             std::pair{"[120, 120]", "[1e-9999, 120]"}}) {
        const auto parsed = avemotion::formats::detail::readOwnJson(replaced(source, mutation.first, mutation.second));
        require(static_cast<bool>(parsed), "underflow source parsed");
        const auto rejected = avemotion::runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
        require(!static_cast<bool>(rejected) && !rejected.prepared && rejected.admission.accepted()
            && rejected.code == avemotion::runtime::detail::OwnNativeEllipseModelCode::UnsupportedNumericConversion,
            "numeric underflow has separate factory failure");
    }
}

void evaluatorAndBinderIsolationContractsHold() {
    const auto source = avemotion::test::ellipseFixture();
    auto linear = replaced(source, "\"x\": 0.667", "\"x\": 1");
    linear = replaced(std::move(linear), "\"x\": 0.333", "\"x\": 0");
    const auto prepared = preparedFrom(linear);
    avemotion::evaluation::PropertyEvaluator evaluator{prepared->model};
    avemotion::evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    const std::array<double, 7> frames{60, 0, 30, 30, 1, 59, 0};
    const std::array<MotionVec2Value, 7> positions{MotionVec2Value{76, 0},
        MotionVec2Value{-76, 0}, MotionVec2Value{0, 0}, MotionVec2Value{0, 0},
        MotionVec2Value{-73.466667F, 0}, MotionVec2Value{73.466667F, 0}, MotionVec2Value{-76, 0}};
    std::array<MotionVec2Value, 7> retained{};
    for (std::size_t index = 0; index < frames.size(); ++index) {
        const auto view = evaluator.evaluate(frames[index], workspace);
        require(static_cast<bool>(view), "linear evaluator request succeeds");
        retained[index] = view.properties[4].value.vec2;
        require(std::abs(retained[index].x - positions[index].x) <= 0.00001F
            && retained[index].y == positions[index].y, "literal linear evaluator sample");
        if (index == 2 || index == 3 || index == 0 || index == 1 || index == 6)
            require(retained[index] == positions[index], "literal linear endpoint/sample");
        require(view.nodeTransforms[1].localMatrix == avemotion::model::MotionMatrix3x2Value{
            1, 0, 0, 1, 256, 256} && view.nodeTransforms[1].worldMatrix
            == avemotion::model::MotionMatrix3x2Value{1, 0, 0, 1, 256, 256}
            && view.nodeTransforms[1].localOpacity == 1.0F && view.nodeTransforms[1].worldOpacity == 1.0F,
            "layer local and world transform");
        require(view.nodeTransforms[2].localMatrix == avemotion::model::MotionMatrix3x2Value{}
            && view.nodeTransforms[2].worldMatrix == avemotion::model::MotionMatrix3x2Value{
                1, 0, 0, 1, 256, 256} && view.nodeTransforms[2].worldOpacity == 1.0F,
            "group local and world transform");
    }
    require(retained[0] == MotionVec2Value{76, 0} && retained[1] == MotionVec2Value{-76, 0},
        "borrowed views copied before later evaluation");
    auto linearExpected = baselineValues();
    linearExpected.outgoing = {0, 0};
    linearExpected.incoming = {1, 1};
    avemotion::test::assertOwnAuthoredModel(*prepared, expectedLinearInput(), linearExpected, linear);

    require(static_cast<bool>(avemotion::runtime::detail::bindNativeEllipseModel(
        *prepared->input, *prepared->model)), "binder accepts own authored model");
    auto wrongOwner = *prepared->model;
    wrongOwner.properties[4].owner = avemotion::model::makeId<avemotion::model::SourceNodeId>(4);
    require(!avemotion::runtime::detail::bindNativeEllipseModel(*prepared->input, wrongOwner),
        "binder rejects wrong property owner");
    auto aliasedScalar = *prepared->model;
    aliasedScalar.properties[3].staticValue = aliasedScalar.properties[1].staticValue;
    require(!avemotion::runtime::detail::bindNativeEllipseModel(*prepared->input, aliasedScalar),
        "binder rejects aliased scalar value");
    auto extraRow = *prepared->model;
    extraRow.scalarValues.push_back(100.0F);
    require(!avemotion::runtime::detail::bindNativeEllipseModel(*prepared->input, extraRow),
        "binder rejects extra typed row");
    auto reordered = *prepared->model;
    std::swap(reordered.sourceChildIds[2], reordered.sourceChildIds[3]);
    require(!avemotion::runtime::detail::bindNativeEllipseModel(*prepared->input, reordered),
        "binder rejects reordered child edge");
    auto reindexed = *prepared->model;
    std::swap(reindexed.properties[0], reindexed.properties[1]);
    reindexed.properties[0].id = avemotion::model::makeId<avemotion::model::PropertyId>(0);
    reindexed.properties[1].id = avemotion::model::makeId<avemotion::model::PropertyId>(1);
    require(static_cast<bool>(avemotion::runtime::detail::bindNativeEllipseModel(*prepared->input, reindexed)),
        "binder accepts semantic property row reindexing");

    const auto other = preparedFrom(replaced(replaced(source, "[256, 256, 0]", "[11, 22, 0]"),
        "\"Moving Circle\"", "\"B\""));
    avemotion::evaluation::PropertyEvaluator otherEvaluator{other->model};
    const auto wrongWorkspace = otherEvaluator.evaluate(0.0, workspace);
    require(wrongWorkspace.error == avemotion::evaluation::PropertyEvaluationErrorCode::WorkspaceNotPrepared,
        "workspace rejects different prepared model");
    avemotion::evaluation::PropertyEvaluationWorkspace otherWorkspace;
    otherEvaluator.prepare(otherWorkspace);
    require(static_cast<bool>(otherEvaluator.evaluate(0.0, otherWorkspace)),
        "separately prepared model workspace succeeds");
}

void parallelModelsKeepDocumentsAndWorkspacesIsolated() {
    struct Result { bool ok = false; MotionVec2Value translation; std::string name; };
    const auto source = avemotion::test::ellipseFixture();
    const auto make = [&](const char* name, const char* translation) {
        return replaced(replaced(source, "\"Moving Circle\"", std::string{"\""} + name + "\""),
            "[256, 256, 0]", translation);
    };
    std::array<Result, 2> results;
    const std::array<std::pair<std::string, std::string>, 2> cases{{{"A", "[11, 22, 0]"}, {"B", "[33, 44, 0]"}}};
    std::array<std::thread, 2> workers;
    for (std::size_t index = 0; index < workers.size(); ++index) {
        workers[index] = std::thread([&, index] {
            for (int run = 0; run < 64; ++run) {
                const auto prepared = preparedFrom(make(cases[index].first.c_str(), cases[index].second.c_str()));
                avemotion::evaluation::PropertyEvaluator evaluator{prepared->model};
                avemotion::evaluation::PropertyEvaluationWorkspace workspace;
                evaluator.prepare(workspace);
                const auto view = evaluator.evaluate(30.0, workspace);
                if (!view || prepared->model->sourceNodes[1].debugName != cases[index].first) return;
                results[index] = {true, prepared->values.translation, prepared->model->sourceNodes[1].debugName};
            }
        });
    }
    for (auto& worker : workers) worker.join();
    require(results[0].ok && results[0].translation == MotionVec2Value{11, 22} && results[0].name == "A"
        && results[1].ok && results[1].translation == MotionVec2Value{33, 44} && results[1].name == "B",
        "parallel models retain independent inputs and workspaces");
}

} // namespace

int main() {
    try {
        const auto run = [](const char* name, const auto& test) {
            try { test(); }
            catch (const std::exception& error) { throw std::runtime_error(std::string{name} + ": " + error.what()); }
        };
        run("baseline", ownAuthoredModelIsPrepared);
        run("failures", ownFactoryKeepsAdmissionAndNumericFailuresSeparate);
        run("static", staticBoundaryModelUsesStaticPositionAndNoTrack);
        run("boundaries", authoredBoundaryAndNameMatrixIsIndependent);
        run("animation", authoredAnimationAndFailureMatrixIsIndependent);
        run("evaluator", evaluatorAndBinderIsolationContractsHold);
        run("parallel", parallelModelsKeepDocumentsAndWorkspacesIsolated);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
