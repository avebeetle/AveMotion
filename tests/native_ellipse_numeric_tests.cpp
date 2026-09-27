#include "NativeEllipseNumeric.hpp"
#include "NativeEllipseAdmissionTestData.hpp"

#include <cstdlib>
#include <cmath>
#include <limits>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void baselineNumericValuesAreInterpreted() {
    const auto values = avemotion::runtime::detail::interpretNativeEllipseInput(
        avemotion::test::expectedEllipseBaseline());
    require(values.has_value(), "baseline numeric values interpreted");
    require(values->frameRate == static_cast<float>(60.0), "baseline frame rate");
    require(values->translation == avemotion::model::MotionVec2Value{256.0F, 256.0F},
        "baseline translation");
    require(values->size == avemotion::model::MotionVec2Value{120.0F, 120.0F},
        "baseline size");
    require(values->start == avemotion::model::MotionVec2Value{-76.0F, 0.0F},
        "baseline start");
    require(values->end == avemotion::model::MotionVec2Value{76.0F, 0.0F},
        "baseline end");
    require(values->outgoing == avemotion::model::MotionVec2Value{
        static_cast<float>(0.333), 0.0F}, "baseline outgoing control");
    require(values->incoming == avemotion::model::MotionVec2Value{
        static_cast<float>(0.667), 1.0F}, "baseline incoming control");
    require(values->color == avemotion::model::MotionColorValue{
        static_cast<float>(0.08), static_cast<float>(0.72), static_cast<float>(0.95), 1.0F},
        "baseline color");
    require(values->animated && values->firstFrame == 0 && values->lastFrame == 60,
        "baseline animation range");
}

void numericRepresentabilityAndInputLimitsAreEnforced() {
    using avemotion::runtime::detail::NativeEllipseAnimatedPosition;
    using avemotion::runtime::detail::NativeEllipseStaticPosition;
    using avemotion::test::ellipseDecimal;
    using avemotion::test::expectedEllipseBaseline;

    auto rejected = expectedEllipseBaseline();
    rejected.frameRate = ellipseDecimal(false, "1", true, "9999");
    require(!avemotion::runtime::detail::interpretNativeEllipseInput(rejected),
        "frame rate underflow rejected");

    rejected = expectedEllipseBaseline();
    rejected.size[0] = ellipseDecimal(false, "1", true, "9999");
    require(!avemotion::runtime::detail::interpretNativeEllipseInput(rejected),
        "size underflow rejected");

    rejected = expectedEllipseBaseline();
    rejected.fillColor[0] = ellipseDecimal(false, "1", true, "9999");
    require(!avemotion::runtime::detail::interpretNativeEllipseInput(rejected),
        "color underflow rejected");

    for (const auto bad : {ellipseDecimal(false, "01"), ellipseDecimal(false, "1", false, "00"),
             ellipseDecimal(false, "1", true, "0"), ellipseDecimal(true, "0"),
             ellipseDecimal(false, "1", false, "9999")}) {
        rejected = expectedEllipseBaseline();
        rejected.size[0] = bad;
        require(!avemotion::runtime::detail::interpretNativeEllipseInput(rejected),
            "malformed numeric decimal rejected");
    }

    auto smallest = expectedEllipseBaseline();
    smallest.size[0] = ellipseDecimal(false, "1", true, "45");
    const auto denormal = avemotion::runtime::detail::interpretNativeEllipseInput(smallest);
    require(denormal && denormal->size.x == std::numeric_limits<float>::denorm_min(),
        "smallest representable size preserved");

    auto rate = expectedEllipseBaseline();
    rate.frameRate = ellipseDecimal(false, "5994", true, "2");
    const auto rateValues = avemotion::runtime::detail::interpretNativeEllipseInput(rate);
    require(rateValues && rateValues->frameRate == static_cast<float>(59.94),
        "decimal frame rate casts through double");

    auto signedZero = expectedEllipseBaseline();
    auto& motion = std::get<NativeEllipseAnimatedPosition>(signedZero.position);
    motion.start[1] = ellipseDecimal(false, "0");
    const auto zeroValues = avemotion::runtime::detail::interpretNativeEllipseInput(signedZero);
    require(zeroValues && zeroValues->start.y == 0.0F && !std::signbit(zeroValues->start.y),
        "accepted zero normalized to positive zero");

    auto staticPosition = expectedEllipseBaseline();
    staticPosition.position = NativeEllipseStaticPosition{{ellipseDecimal(true, "32768"),
        ellipseDecimal(false, "32768")}};
    const auto staticValues = avemotion::runtime::detail::interpretNativeEllipseInput(staticPosition);
    require(staticValues && !staticValues->animated
        && staticValues->start == avemotion::model::MotionVec2Value{-32768.0F, 32768.0F},
        "static position boundary interpreted");
}

} // namespace

int main() {
    baselineNumericValuesAreInterpreted();
    numericRepresentabilityAndInputLimitsAreEnforced();
}
