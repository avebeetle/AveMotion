#include "NativeEllipseAdmissionCore.hpp"
#include "support/OwnVectorGradientTestData.hpp"
#include "OwnVectorTestData.hpp"
#include "support/OwnVectorGroupTestData.hpp"
#include "support/OwnVectorMetadataPositionTestData.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <tuple>
using namespace avemotion;
using test::vectorRequire;
using namespace model;
using test::vectorLayer;
using test::vectorPrecomp;
using test::vectorReplace;
using test::vectorRoot;
using test::vectorShape;
using test::vectorShapeLayer;
void near(float got, float expected, const std::string& label) {
    vectorRequire(std::abs(got - expected) <= 0.0001F, label + " got=" + std::to_string(got));
}
void exporterMetadata() {
    using namespace test;
    const auto plain = vectorRoot(clippingShape(24, 24, 48, 48));
    const auto annotated = vectorReplace(metadataPositionAnnotate(plain),
        R"("markers":[{"tm":25.5,"cm":"owned marker","dr":0.25}],)", "");
    const auto original = vectorCompile(plain), admitted = vectorCompile(annotated);
    vectorRequire(bool(admitted), "bounded exporter metadata admitted: " + admitted.path + admitted.message);
    const auto& a = *original.prepared->model;
    const auto& b = *admitted.prepared->model;
    vectorRequire(a.sourceNodes.size() == b.sourceNodes.size() && a.properties.size() == b.properties.size() &&
                  a.tracks.size() == b.tracks.size() && original.prepared->draws.size() == admitted.prepared->draws.size(),
                  "discarded annotations leave model inventory unchanged");
    for (const auto& [needle, kind] : std::vector<std::pair<std::string,std::string>>{
             {R"("ty":"gr")", "group"}, {R"("ty":"sh")", "path"}}) {
        for (const auto& key : kind == "group" ? std::vector<std::string>{"np","cix","ix"}
                                             : std::vector<std::string>{"ix"}) {
            for (const auto& value : {"0", "32768", "3e0"})
                vectorRequire(bool(vectorCompile(vectorReplace(plain, needle, needle + ",\"" + key + "\":" + value))),
                              "inclusive exporter index: " + key);
            for (const auto& value : {"-1", "-0.0000001", "32768.00000001", "0.5", "1.0000000000000001", "true", "\"3\"", "null"})
                metadataPositionReject(vectorReplace(plain, needle, needle + ",\"" + key + "\":" + value), kind + key + value);
        }
    }
    for (const auto& needle : {R"("ty":"gr")", R"("ty":"sh")", R"("ty":"fl")"}) {
        vectorRequire(bool(vectorCompile(vectorReplace(plain, needle, std::string(needle) + ",\"mn\":\"" + std::string(1024,'x') + "\""))),
                      "1024 byte exporter name");
        for (const auto& value : {std::string("\"") + std::string(1025,'x') + "\"", std::string("7"),std::string("null")})
            metadataPositionReject(vectorReplace(plain, needle, std::string(needle) + ",\"mn\":" + value), "name bound/type");
        for (const auto& field : {R"("l2":0)", R"("unwitnessed":0)"})
            metadataPositionReject(vectorReplace(plain, needle, std::string(needle) + "," + field), "strict unknown shape field");
    }
    for (const auto& [needle,field] : std::vector<std::pair<std::string,std::string>>{
             {R"("fr":60)", R"("mn":"bad")"}, {R"("ty":4)", R"("ix":1)"},
             {R"("ty":"sh")", R"("np":1)"}, {R"("ty":"fl")", R"("ix":1)"},
             {R"("ty":"tr")", R"("mn":"bad")"}, {R"("k":[1,0,0,1])",R"("mn":"bad")"},
             {R"("ty":"gr")", R"("markers":[])"}, {R"("ty":4)",R"("markers":[])"},
             {R"("ty":"sh")",R"("markers":[])"}, {R"("ty":"tr")",R"("markers":[])"},
             {R"("k":[1,0,0,1])",R"("markers":[])"}})
        metadataPositionReject(vectorReplace(plain, needle, needle + "," + field), "wrong metadata context");
    // Existing property ix domain remains strict, independently of item metadata.
    metadataPositionReject(vectorRoot(vectorLayer(R"("r":{"k":0,"ix":0.5})")), "property ix unchanged");
    metadataPositionReject(vectorReplace(vectorRoot(clippingShape(24,24,48,48,true)),
                           R"("ty":"st")",R"("ty":"st","mn":"wrong context")"),"stroke exporter metadata excluded");
    metadataPositionReject(vectorReplace(vectorPrecomp(vectorLayer("")),R"("id":"x")",
                           R"("id":"x","markers":[])"),"asset markers excluded");
    const auto markers = [&](const std::string& value) {
        return vectorReplace(plain, R"("fr":60)", "\"markers\":" + value + R"(,"fr":60)");
    };
    for (unsigned count : {0U,128U,129U}) {
        std::string entries;
        for (unsigned i=0;i<count;++i) { if(i) entries += ','; entries += R"({"tm":-20000,"cm":"","dr":20000})"; }
        const auto result = vectorCompile(markers("[" + entries + "]"));
        vectorRequire(bool(result) == (count <= 128) && (count <= 128 || !result.prepared), "marker array budget");
    }
    vectorRequire(bool(vectorCompile(markers("[{\"tm\":20000,\"cm\":\"" + std::string(1024,'m') + "\",\"dr\":0}]"))),
                  "inclusive marker boundaries and byte cap");
    for (const auto& marker : {R"({"tm":-20000.0001,"cm":"x","dr":0})", R"({"tm":20000.0001,"cm":"x","dr":0})",
             R"({"tm":0,"cm":"x","dr":-0.0001})", R"({"tm":0,"cm":"x","dr":20000.0001})",
             R"({"tm":"0","cm":"x","dr":0})", R"({"tm":0,"cm":0,"dr":0})",
             R"({"tm":0,"cm":"x","dr":true})", R"({"cm":"x","dr":0})", R"({"tm":0,"dr":0})",
             R"({"tm":0,"cm":"x"})", R"({"tm":0,"cm":"x","dr":0,"unknown":0})",
             R"({"tm":1e999,"cm":"x","dr":0})",R"({"tm":0,"cm":"x","dr":1e999})", "null"})
        metadataPositionReject(markers("[" + std::string(marker) + "]"), "malformed marker");
    metadataPositionReject(markers("[{\"tm\":0,\"cm\":\"" + std::string(1025,'m') + "\",\"dr\":0}]"), "marker name cap");
    metadataPositionReject(markers("{}"), "marker array type");
}
void exporterMarkerAdmission() {
    using namespace test;
    const auto result = vectorCompile(vectorReplace(vectorRoot(vectorLayer("")), R"("fr":60)",
        R"("markers":[{"tm":25.5,"cm":"owned marker","dr":0.25}],"fr":60)"));
    vectorRequire(bool(result), "bounded root marker admitted: " + result.path + result.message);
    vectorRequire(result.prepared->model->sourceNodes.size() == 2 && result.prepared->model->properties.size() == 5 &&
                  result.prepared->model->tracks.empty(), "markers produce no model records");
}
void separatedPosition() {
    using namespace test;
    const auto position = [&](const std::string& axes) { return vectorRoot(vectorLayer("\"p\":" + axes)); };
    for (const auto& axes : {std::string(R"({"s":true,"x":{"k":-32768},"y":{"k":32768}})"),
                             metadataPositionAxes(), std::string(R"({"y":{"k":4},"s":true,"x":{"k":3}})"),
                             std::string(R"({"x":{"k":3},"y":{"k":4},"s":true})"),
                             std::string(R"({"s":true,"y":{"k":4},"x":{"k":3}})"),
                             std::string(R"({"x":{"k":3},"s":true,"y":{"k":4}})"),
                             std::string(R"({"y":{"k":4},"x":{"k":3},"s":true})")}) {
        const auto result = vectorCompile(position(axes));
        vectorRequire(bool(result), "separated layer axes admitted: " + result.path + result.message);
        const bool animated = axes == metadataPositionAxes();
        unsigned axesFound = 0;
        for (const auto& property : result.prepared->model->properties) {
            vectorRequire(property.semantic != PropertySemantic::TransformPosition, "split emits no combined position");
            if (property.semantic == PropertySemantic::TransformPositionX || property.semantic == PropertySemantic::TransformPositionY) {
                ++axesFound;
                vectorRequire(property.valueType == PropertyValueType::Scalar && property.flags ==
                              (PropertyFlagSeparatedDimension | (animated ? PropertyFlagAnimated : PropertyFlagStatic)),
                              "split scalar flags retain animation/static and separated state");
            }
        }
        vectorRequire(axesFound == 2, "exactly two axis records");
        evaluation::PropertyEvaluator evaluator(result.prepared->model);
        evaluation::PropertyEvaluationWorkspace ws;
        evaluator.prepare(ws);
        if (animated) {
            for (const auto& sample : std::vector<std::tuple<double,float,float>>{
                    {-1,0.F,0.F}, {0,0.F,0.F}, {14,7.F,0.F}, {14.5,7.25F,0.F}, {15,7.5F,10.F},
                    {20,10.F,10.F}, {40,20.F,10.F}, {59,20.F,10.F}, {60,20.F,-5.F}, {179,20.F,-5.F}, {15,7.5F,10.F}, {0,0.F,0.F}}) {
                const auto [frame,x,y] = sample;
                const auto view = evaluator.evaluate(frame,ws);
                vectorRequire(bool(view), "axis evaluation");
                const auto& matrix = view.nodeTransforms[result.prepared->layers[0].layer.index()].worldMatrix;
                near(matrix.dx,x,"literal independent X sample"); near(matrix.dy,y,"literal independent Y hold sample");
            }
        } else {
            const auto view = evaluator.evaluate(17,ws);
            const auto& matrix = view.nodeTransforms[result.prepared->layers[0].layer.index()].worldMatrix;
            near(matrix.dx, axes.find("-32768") != std::string::npos ? -32768.F : 3.F,"static X");
            near(matrix.dy, axes.find("-32768") != std::string::npos ? 32768.F : 4.F,"static Y");
        }
    }
    for (const auto& axes : {R"({"s":true,"x":{"k":0}})", R"({"s":true,"y":{"k":0}})",
             R"({"s":false,"x":{"k":0},"y":{"k":0}})", R"({"s":1,"x":{"k":0},"y":{"k":0}})",
             R"({"x":{"k":0},"y":{"k":0}})", R"({"s":true,"x":{"k":0},"y":{"k":0},"k":[0,0]})",
             R"({"s":true,"x":{"k":0},"y":{"k":0},"z":{"k":0}})", R"({"s":true,"x":{"k":0},"y":{"k":0},"other":0})",
             R"({"s":true,"x":{"k":[0]},"y":{"k":0}})", R"({"s":true,"x":{"k":null},"y":{"k":0}})",
             R"({"s":true,"x":{"k":-32768.000001},"y":{"k":0}})", R"({"s":true,"x":{"k":0},"y":{"k":32768.000001}})"})
        metadataPositionReject(position(axes), "split keys/type/domain");
    for (const auto& [from,to] : std::vector<std::pair<std::string,std::string>>{
             {R"("t":40)",R"("t":0)"}, {R"("t":40)",R"("t":-1)"}, {R"("t":40)",R"("t":20001)"},
             {R"("s":[20])",R"("s":[32768.000001])"}, {R"("s":[20])",R"("s":20)"},
             {R"("t":0,"s":[0],"o":)", R"("t":0,"s":[0],"ti":[0,0],"to":[0,0],"o":)"},
             {R"("x":1,"y":1)",R"("x":1.01,"y":1)"}})
        metadataPositionReject(position(vectorReplace(metadataPositionAxes(),from,to)),"split track validation");
    metadataPositionReject(vectorRoot(vectorLayer("\"p\":" + metadataPositionAxes(),R"(,"ao":1)")),"auto orient unchanged");
    metadataPositionReject(vectorRoot(vectorReplace(clippingShape(24,24,48,48),R"("ty":"tr")",
                           R"("ty":"tr","p":)" + metadataPositionAxes())),"group split rejected");
    metadataPositionReject(vectorReplace(vectorPrecomp(vectorLayer("")),R"("op":170,"ks":{})",
                           R"("op":170,"ks":{"p":)" + metadataPositionAxes() + "}"),"precomp split rejected");
}
void boundedGroups() {
    using namespace test;
    for (const unsigned count : {3U, 10U, 128U}) {
        const auto result = vectorCompile(vectorRoot(vectorGroupsLayer(vectorGroups(count))));
        vectorRequire(bool(result), "sibling groups " + std::to_string(count) + ": " + result.path +
                                        result.message);
        vectorRequire(result.prepared->draws.size() == count, "all sibling paints retained");
        vectorRequire(
            result.prepared->sources[result.prepared->draws.front().path.index()].jsonPointer ==
                "/layers/0/shapes/" + std::to_string(count - 1) + "/it/0",
            "reverse sibling paint order");
    }
    const auto reject = [](const std::string &json, const std::string &label) {
        const auto result = vectorCompile(json);
        vectorRequire(
            !result && !result.prepared && !result.path.empty() && !result.message.empty(), label);
        return result;
    };
    const auto items = reject(vectorRoot(vectorGroupsLayer(vectorGroups(129))),
                              "129 top-level items atomic rejection");
    vectorRequire(items.path == "/layers/0/shapes", "item budget diagnostic identifies shape list");
    const auto paths = reject(vectorRoot(vectorGroupsLayer(vectorGroups(64)) + "," +
                                         vectorGroupsLayer(vectorGroups(65), 2)),
                              "asset-wide 129 paths");
    vectorRequire(paths.path == "/layers/1/shapes/64/it/0" &&
                      paths.message == "path count limit exceeded",
                  "asset-wide path budget remains enforced");
}
void groupOpacity() {
    using namespace test;
    const auto reject = [](const std::string &json, const std::string &label) {
        const auto result = vectorCompile(json);
        vectorRequire(
            !result && !result.prepared && !result.path.empty() && !result.message.empty(), label);
    };
    for (const auto &opacity :
         {std::string(""), std::string(R"({"k":0})"), std::string(R"({"k":50})"),
          std::string(R"({"k":100})"), vectorGroupLinearOpacity(), vectorGroupHeldOpacity()}) {
        const auto result = vectorCompile(vectorRoot(vectorGroupsLayer(vectorGroup(opacity))));
        vectorRequire(bool(result),
                      "group opacity admitted: " + opacity + result.path + result.message);
        evaluation::PropertyEvaluator evaluator(result.prepared->model);
        evaluation::PropertyEvaluationWorkspace ws;
        evaluator.prepare(ws);
        const auto group = result.prepared->draws[0].group;
        if (opacity == vectorGroupHeldOpacity()) {
            for (const auto [frame, expected] :
                 {std::pair{68., 0.F}, {69., 0.5F}, {110., 1.F}, {68., 0.F}}) {
                const auto view = evaluator.evaluate(frame, ws);
                near(view.nodeTransforms[group.index()].worldOpacity, expected,
                     "held group opacity boundary");
            }
        } else {
            const auto view = evaluator.evaluate(50, ws);
            const float expected = opacity.empty() || opacity == R"({"k":100})" ? 1.F
                                   : opacity == R"({"k":0})"                    ? 0.F
                                                                                : 0.5F;
            near(view.nodeTransforms[group.index()].worldOpacity, expected,
                 "literal static/linear group opacity");
        }
    }
    for (const auto &opacity : {R"({"k":-0.000001})", R"({"k":100.000001})",
                                R"({"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":10,"s":[101]}]})"})
        reject(vectorRoot(vectorGroupsLayer(vectorGroup(opacity))), "out-of-range group opacity");
    for (const auto &opacity : {R"({"k":0})", R"({"k":50})",
                                R"({"a":1,"k":[{"t":0,"s":[100],"h":1},{"t":10,"s":[100]}]})"})
        reject(vectorReplace(vectorPrecomp(vectorLayer("")), "\"op\":170,\"ks\":{}",
                             "\"op\":170,\"ks\":{\"o\":" + std::string(opacity) + "}"),
               "precomp opacity unchanged");
    const auto stroke = vectorGroup("", 0, true);
    const auto both =
        vectorReplace(stroke, "{\"ty\":\"tr\"}", R"({"ty":"fl","c":{"k":[0,1,0,1]}},{"ty":"tr"})");
    const auto result = vectorCompile(vectorRoot(vectorGroupsLayer(both)));
    vectorRequire(
        bool(result) && result.prepared->draws.size() == 2 &&
            result.prepared->model->sourceNodes[result.prepared->draws[0].paint.index()].kind ==
                SourceNodeKind::Fill &&
            result.prepared->model->sourceNodes[result.prepared->draws[1].paint.index()].kind ==
                SourceNodeKind::Stroke,
        "fill precedes stroke in emitted paint order");
}
void exactAdmission() {
    vectorRequire(runtime::detail::equalOwnNumericTokens("100", "1e2") &&
                      runtime::detail::equalOwnNumericTokens("0.5", "5e-1") &&
                      runtime::detail::equalOwnNumericTokens("0", "-0e999999999999999999999"),
                  "exact helper accepts equivalent numeric spellings");
    for (const auto& invalid : {"", "nan", "+1", ".5", "1e", "1x"})
        vectorRequire(!runtime::detail::equalOwnNumericTokens(invalid, invalid),
                      "exact equality validates both JSON number tokens");
    const auto group =
        vectorRoot(vectorShapeLayer("{\"a\":0,\"k\":" + vectorShape(2, false) + "}"));
    const auto precomp = vectorPrecomp(vectorLayer(""));
    const auto precompTransform = [&](const std::string& properties) {
        return vectorReplace(precomp, "\"op\":170,\"ks\":{}",
                             "\"op\":170,\"ks\":{" + properties + "}");
    };
    const auto easing = [](const std::string& controls) {
        return vectorRoot(vectorLayer("\"r\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[0],\"i\":" + controls +
                                      ",\"o\":{\"x\":0,\"y\":0}},{\"t\":20,\"s\":[30]}]}"));
    };
    const std::vector<std::pair<std::string, std::string>> negatives{
        {"group rounded scale",
         vectorReplace(group, "\"ty\":\"tr\"", R"("ty":"tr","s":{"a":0,"k":[100.000001,100]})")},
        {"precomp rounded scale", precompTransform(R"("s":{"a":0,"k":[100,100.000001]})")},
        {"rounded easing x equality", easing(R"({"x":[0.5,0.500000001],"y":[1,1]})")},
        {"rounded easing y equality", easing(R"({"x":[0.5,0.5],"y":[0.5,0.500000001]})")}};
    std::string failures;
    for (const auto& [label, json] : negatives) {
        const auto result = test::vectorCompile(json);
        if (result || result.prepared || result.path.empty() || result.message.empty())
            failures += label + "; ";
    }
    const std::vector<std::pair<std::string, std::string>> positives{
        {"equivalent group scale",
         vectorReplace(group, "\"ty\":\"tr\"", R"("ty":"tr","s":{"a":0,"k":[100,1e2]})")},
        {"equivalent precomp scale", precompTransform(R"("s":{"a":0,"k":[1e2,100.0]})")},
        {"equivalent precomp identity",
         precompTransform(R"("p":{"a":0,"k":[1e2,0.5]},"a":{"a":0,"k":[100,5e-1]})")},
        {"equivalent easing channels", easing(R"({"x":[0.5,5e-1],"y":[1,1e0]})")},
        {"ordinary layer scale remains unrestricted",
         vectorRoot(vectorLayer(R"("s":{"a":0,"k":[100.000001,125]})"))},
        {"missing anchor means exact zero", precompTransform(R"("p":{"a":0,"k":[0e2,-0]})")}};
    for (const auto& [label, json] : positives) {
        const auto result = test::vectorCompile(json);
        vectorRequire(bool(result), label + " " + result.path + " " + result.message);
    }
    vectorRequire(failures.empty(), "exact admission rounded-equality bypasses: " + failures);
}
void properties() {
    const auto json = vectorRoot(vectorLayer(
        R"("r":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":15,"s":[30],"h":1},{"t":180,"s":[360],"h":1}]},"p":{"a":1,"k":[{"t":0,"s":[0,0,0],"o":{"x":0,"y":0},"i":{"x":1,"y":1},"to":[0,10,0],"ti":[0,10,0]},{"t":20,"s":[10,0,0]}]})"));
    auto result = test::vectorCompile(json);
    vectorRequire(bool(result), "holds/spatial compile: " + result.path + " " + result.message);
    evaluation::PropertyEvaluator evaluator(result.prepared->model);
    evaluation::PropertyEvaluationWorkspace ws;
    evaluator.prepare(ws);
    for (const auto& [frame, rotation] :
         {std::pair{14., 0.F}, {15., 30.F}, {179., 30.F}, {180., 360.F}, {200., 360.F}}) {
        auto view = evaluator.evaluate(frame, ws);
        vectorRequire(bool(view), "hold evaluation");
        for (const auto& p : view.properties)
            if (p.semantic == PropertySemantic::TransformRotation)
                near(p.value.scalar, rotation, "hold boundary");
    }
    auto view = evaluator.evaluate(10, ws);
    for (const auto& p : view.properties)
        if (p.semantic == PropertySemantic::TransformPosition) {
            near(p.value.vec2.x, 5, "spatial midpoint X");
            near(p.value.vec2.y, 7.5F, "spatial tangents change midpoint Y");
        }
    view = evaluator.evaluate(200, ws);
    for (const auto& p : view.properties)
        if (p.semantic == PropertySemantic::TransformPosition)
            near(p.value.vec2.x, 10, "ordinary terminal retained");
    auto heldTerminal = test::vectorCompile(
        vectorRoot(vectorLayer(R"("r":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":15,"s":[30]}]})")));
    vectorRequire(bool(heldTerminal), "hold followed by ordinary terminal compiles");
    evaluation::PropertyEvaluator heldEvaluator(heldTerminal.prepared->model);
    heldEvaluator.prepare(ws);
    for (const auto& [frame, expected] : {std::pair{14., 0.F}, {15., 30.F}, {200., 30.F}}) {
        view = heldEvaluator.evaluate(frame, ws);
        for (const auto& p : view.properties)
            if (p.semantic == PropertySemantic::TransformRotation)
                near(p.value.scalar, expected, "hold to ordinary terminal boundary");
    }
}
void measuredNumericAdmission() {
    const auto keyedRotation = vectorRoot(vectorLayer(
        R"("r":{"a":1,"k":[{"t":-12,"s":[0],"h":1},{"t":20,"s":[30]}]})"));
    auto result = test::vectorCompile(keyedRotation);
    vectorRequire(bool(result), "signed bounded key time: " + result.path + " " + result.message);
    const auto scale = vectorRoot(vectorLayer(
        R"("s":{"a":1,"k":[{"t":0,"s":[100,100,100],"i":{"x":[0.75,0.75,0.9],"y":[1,1,-3.532]},"o":{"x":[0.25,0.25,0.1],"y":[0,0,7.017]}},{"t":10,"s":[90,90,100]}]})"));
    result = test::vectorCompile(scale);
    vectorRequire(bool(result), "neutral Z easing ignored: " + result.path + " " + result.message);
    const std::string width = R"({"ty":"st","c":{"a":0,"k":[0,0,1,1]},"w":{"a":1,"k":[{"t":0,"s":[0],"i":{"x":0.75,"y":2},"o":{"x":0.25,"y":2}},{"t":10,"s":[10]}]}})";
    const auto strokeJson = vectorRoot(vectorReplace(
        vectorShapeLayer("{\"a\":0,\"k\":" + vectorShape(2, false) + "}"),
        "{\"ty\":\"tr\"}", width + ",{\"ty\":\"tr\"}"));
    result = test::vectorCompile(strokeJson);
    vectorRequire(bool(result), "stroke width Y overshoot admitted: " + result.path + " " + result.message);
    evaluation::PropertyEvaluator evaluator(result.prepared->model);
    evaluation::PropertyEvaluationWorkspace ws;
    evaluator.prepare(ws);
    const auto view = evaluator.evaluate(5, ws);
    vectorRequire(bool(view), "overshoot width evaluates");
    for (const auto& p : view.properties)
        if (p.semantic == PropertySemantic::StrokeWidth)
            near(p.value.scalar, 16.25F, "literal width overshoot at midpoint");
    for (const auto& [name, json] : std::vector<std::pair<std::string, std::string>>{
             {"negative time below bound", vectorReplace(keyedRotation, "\"t\":-12", "\"t\":-20001")},
             {"width Y above bound", vectorReplace(strokeJson, "\"y\":2", "\"y\":16.001")},
             {"width X outside unit", vectorReplace(strokeJson, "\"x\":0.75", "\"x\":1.001")},
             {"visible scale Y overshoot", vectorReplace(scale, "\"y\":[1,1,-3.532]", "\"y\":[1.01,1.01,-3.532]")},
             {"neutral Z Y below bound", vectorReplace(scale, "-3.532", "-16.001")},
             {"neutral Z X outside unit", vectorReplace(scale, "0.9", "1.001")}}) {
        const auto bad = test::vectorCompile(json);
        vectorRequire(!bad && !bad.prepared, "exact numeric rejection: " + name);
    }
}
void measuredScopeRejections() {
    const auto shape = vectorShape(2, false);
    const auto path = std::string("{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":") + shape + "}}";
    const std::string stroke = R"({"ty":"st","c":{"a":0,"k":[1,0,0,1]},"w":{"a":0,"k":2}})";
    const std::string trim = R"({"ty":"tm","m":1,"s":{"a":0,"k":0},"e":{"a":0,"k":50}})";
    const auto group = std::string("{\"ty\":\"gr\",\"it\":[") + path + "," + stroke + R"(,{"ty":"tr"}]})";
    const auto root = [&](const std::string& items) {
        return vectorRoot("{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":180,\"ks\":{},\"shapes\":[" + items + "]}");
    };
    for (const auto& [name, json] : std::vector<std::pair<std::string, std::string>>{
             {"two effective trims", root(vectorReplace(group, stroke, trim + "," + stroke) + "," + trim)},
             {"nested group", root(vectorReplace(group, path, group))},
             {"outer multi-path paint", root(group + "," + group + "," + stroke)},
             {"individual trim", root(group + "," + vectorReplace(trim, "\"m\":1", "\"m\":2"))},
             {"nonuniform scale", root(vectorReplace(group, "{\"ty\":\"tr\"}", R"({"ty":"tr","s":{"a":0,"k":[75,76]}})"))},
             {"zero scale", root(vectorReplace(group, "{\"ty\":\"tr\"}", R"({"ty":"tr","s":{"a":0,"k":[0,0]}})"))},
             {"oversize scale", root(vectorReplace(group, "{\"ty\":\"tr\"}", R"({"ty":"tr","s":{"a":0,"k":[1000.001,1000.001]}})"))}}) {
        const auto bad = test::vectorCompile(json);
        vectorRequire(!bad && !bad.prepared && !bad.path.empty(), "bounded scope rejection: " + name);
    }
}
void morphsAndDefaults() {
    for (bool closed : {false, true}) {
        const auto start = vectorShape(2, closed), end = vectorShape(2, closed, 20);
        const auto path = "{\"a\":1,\"k\":[{\"t\":0,\"s\":[" + start +
                          "],\"i\":{\"x\":1,\"y\":1},\"o\":{\"x\":0,\"y\":0}},{\"t\":20,\"s\":[" +
                          end + "]}]}";
        auto result = test::vectorCompile(vectorRoot(vectorShapeLayer(path)));
        vectorRequire(bool(result), "open/closed morph compile");
        evaluation::PropertyEvaluator evaluator(result.prepared->model);
        evaluation::PropertyEvaluationWorkspace ws;
        evaluator.prepare(ws);
        auto view = evaluator.evaluate(10, ws);
        vectorRequire(bool(view) && view.shapes.size() == 1, "morph materialized");
        const auto& shape = view.shapes[0];
        vectorRequire(!shape.closed && !shape.topologyTruncated &&
                          shape.pointCount == (closed ? 7U : 4U),
                      "pinned interpolated morph closure semantics");
        near(view.shapePoints[shape.firstPoint].x, 10, "hand-derived morph midpoint");
        const auto& transform = view.nodeTransforms[result.prepared->layers[0].layer.index()];
        vectorRequire(transform.localMatrix == MotionMatrix3x2Value{} &&
                          transform.localOpacity == 1,
                      "omitted components neutral");
        view = evaluator.evaluate(30, ws);
        near(view.shapePoints[view.shapes[0].firstPoint].x, 20, "morph terminal retained");
        auto bad = test::vectorCompile(
            vectorRoot(vectorShapeLayer(vectorReplace(path, end, vectorShape(3, closed)))));
        vectorRequire(!bad && !bad.prepared && bad.path.find("/s") != std::string::npos,
                      "morph vertex count rejects atomically");
        bad = test::vectorCompile(
            vectorRoot(vectorShapeLayer(vectorReplace(path, end, vectorShape(2, !closed, 20)))));
        vectorRequire(!bad && !bad.prepared, "morph closure mismatch rejects");
    }
    auto result = test::vectorCompile(vectorPrecomp(vectorLayer("")));
    vectorRequire(bool(result) && result.prepared->layers.size() == 2, "identity precomp expanded");
    vectorRequire(result.prepared->layers[1].inFrame == 10 &&
                      result.prepared->layers[1].outFrame == 170,
                  "child intersects precomp interval");
    const auto& child =
        result.prepared->model->sourceNodes[result.prepared->layers[1].layer.index()];
    vectorRequire(child.parent == result.prepared->layers[0].layer &&
                      child.transformParent == child.parent,
                  "structural container preserved");
    auto outside = test::vectorCompile(
        vectorRoot(vectorReplace(vectorLayer(""), "\"ip\":0,\"op\":180", "\"ip\":190,\"op\":200")));
    vectorRequire(bool(outside) &&
                      outside.prepared->layers[0].inFrame == outside.prepared->layers[0].outFrame,
                  "disjoint visibility interval is empty");
    // Owned JSON/model survive both input string and document destruction.
    vectorRequire(!result.prepared->exactJson.empty() &&
                      runtime::detail::validateOwnVectorModel(*result.prepared),
                  "owned prepared data retained");
}
void rejects(const std::string& fixture) {
    const auto reject = [](const std::string& json, const std::string& role) {
        auto result = test::vectorCompile(json);
        vectorRequire(!result && !result.prepared && !result.path.empty() &&
                          !result.message.empty(),
                      "atomic rejection: " + role);
    };
    for (const auto& controls : {R"({"x":0.5,"y":1.01})", R"({"x":-0.01,"y":0.5})"}) {
        reject(vectorRoot(vectorLayer(
                   "\"r\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[0],\"i\":" + std::string(controls) +
                   ",\"o\":{\"x\":0.3,\"y\":0}},{\"t\":20,\"s\":[30]}]}")),
               "bounded temporal handle x/y");
    }
    reject(vectorReplace(fixture, "\"ty\":\"tr\"", "\"ty\":\"tr\",\"nm\":42"),
           "transform metadata type");
    for (const auto& extra : {",\"masksProperties\":[]", ",\"ef\":[]", ",\"sr\":2", ",\"st\":0.5",
                              ",\"ddd\":1", ",\"unknown\":0", ",\"parent\":1", ",\"parent\":99"})
        reject(vectorRoot(vectorLayer("", extra)), extra);
    reject(vectorReplace(fixture, "\"ind\":9", "\"ind\":7"), "duplicate layer id");
    reject(vectorReplace(fixture, "\"ind\":9", "\"ind\":9,\"parent\":7"), "cyclic parents");
    reject(vectorReplace(fixture, "\"ind\":9", "\"ind\":9.0000000000000000001"),
           "no integer rounding");
    reject(vectorReplace(fixture, "\"k\":90", "\"k\":32768.00000000000000001"),
           "exact component bound");
    reject(vectorReplace(fixture, "\"k\":[200,300]", "\"k\":[1001,300]"), "scale bound");
    reject(vectorReplace(fixture, "\"k\":[10,20]", "\"k\":[10,20,1]"), "position Z");
    reject(vectorReplace(fixture, "\"k\":[200,300]", "\"k\":[200,300,99]"), "scale Z");
    reject(vectorReplace(fixture, "\"r\":1", "\"r\":3"), "fill rule");
    reject(vectorReplace(fixture, "\"lc\":2", "\"lc\":4"), "stroke cap");
    reject(vectorReplace(fixture, "\"w\":{\"a\":0,\"k\":2}", "\"w\":{\"a\":0,\"k\":-0.01}"),
           "stroke negative width");
    reject(vectorReplace(fixture, "\"ty\":\"tr\"", "\"ty\":\"tr\",\"sk\":{\"a\":0,\"k\":1}"),
           "skew");
    reject(vectorReplace(fixture, "\"k\":[3,4]", "\"k\":[3,4],\"x\":\"expression\""), "expression");
    reject(vectorReplace(fixture, "\"c\":false", "\"c\":0"), "closure type");
    reject(
        vectorRoot(vectorLayer(
            R"("s":{"a":1,"k":[{"t":0,"s":[100,100,100],"i":{"x":[0.5,0.6],"y":[1,1]},"o":{"x":0.3,"y":0}},{"t":10,"s":[90,90,100]}]})")),
        "unequal ease axes");
    reject(vectorRoot(vectorLayer(R"("r":{"a":1,"k":[{"t":10,"s":[0],"h":1},{"t":10,"s":[30]}]})")),
           "unordered keys");
    reject(vectorRoot(vectorLayer(R"("r":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":20}]})")),
           "missing terminal value");
    reject(
        vectorRoot(vectorLayer(R"("r":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":20001,"s":[30]}]})")),
        "key time bounds");
    for (const auto& pair : {std::pair{"\"fr\":60", "\"fr\":0"},
                             {"\"w\":128", "\"w\":4097"},
                             {"\"w\":128", "\"w\":127.999999999999999999"},
                             {"\"op\":180", "\"op\":10001"}})
        reject(vectorReplace(vectorRoot(vectorLayer("")), pair.first, pair.second), "root bounds");
    const auto precomp = vectorPrecomp(vectorLayer(""));
    reject(vectorReplace(precomp, "\"refId\":\"x\"", "\"refId\":\"missing\""),
           "missing precomp asset");
    reject(vectorReplace(precomp, "\"ty\":0", "\"ty\":0,\"sr\":0.5"), "precomp clock");
    reject(vectorReplace(precomp, "\"op\":170,\"ks\":{}",
                         "\"op\":170,\"ks\":{\"r\":{\"a\":0,\"k\":1}}"),
           "rotated precomp");
    reject(vectorPrecomp(
               "{\"ty\":0,\"ind\":1,\"ip\":0,\"op\":180,\"refId\":\"x\",\"w\":128,\"h\":128}"),
           "nested precomp/cycle");
    reject(vectorRoot(vectorShapeLayer("{\"a\":0,\"k\":" + vectorShape(257, false) + "}")),
           "vertex count bound");
    std::string many;
    for (int i = 1; i <= 129; ++i) {
        if (i > 1)
            many += ",";
        many += vectorReplace(vectorLayer(""), "\"ind\":1", "\"ind\":" + std::to_string(i));
    }
    reject(vectorRoot(many), "expanded layer bound");
    std::string keys;
    for (int i = 0; i < 257; ++i) {
        if (i)
            keys += ",";
        keys += "{\"t\":" + std::to_string(i) + ",\"s\":[0],\"h\":1}";
    }
    reject(vectorRoot(vectorLayer("\"r\":{\"a\":1,\"k\":[" + keys + "]}")),
           "keys per property bound");
    // 33*256 held segments fit the JSON value ceiling but exceed the compiler
    // segment budget. The first 32 layers establish the inclusive 8192 boundary.
    keys.clear();
    for (int i = 0; i < 256; ++i) {
        if (i)
            keys += ",";
        keys += "{\"t\":" + std::to_string(i) + ",\"s\":[0],\"h\":1}";
    }
    many.clear();
    for (int i = 1; i <= 33; ++i) {
        if (i > 1)
            many += ",";
        many += vectorReplace(vectorLayer("\"r\":{\"a\":1,\"k\":[" + keys + "]}"), "\"ind\":1",
                              "\"ind\":" + std::to_string(i));
        if (i == 32)
            vectorRequire(bool(test::vectorCompile(vectorRoot(many))), "8192 segments accepted");
    }
    reject(vectorRoot(many), "8193+ total segments rejected");
}
void gradientModel() {
    using namespace avemotion::test;
    // Missing gradient admission, wrong packed ranges, or FillColor substitution must fail.
    const auto result = vectorCompile(gradientFixture());
    vectorRequire(bool(result), "owned static gradient admission: " + result.path + " " + result.message);
    const auto &m = *result.prepared->model;
    const auto &n = m.sourceNodes[result.prepared->draws[0].paint.index()];
    vectorRequire(n.kind == SourceNodeKind::GradientFill && n.gradientType == SourceGradientType::Linear &&
                      n.gradientColorPointCount == 3, "canonical linear gradient source");
    vectorRequire(m.gradientValues.size() == 1 && m.gradientFloats.size() == 12 &&
                      m.statistics.gradientValueCount == 1 && m.colorValues.empty(), "packed gradient statistics");
    vectorRequire(m.gradientValues[0].values.first == 0 && m.gradientValues[0].values.count == 12 &&
                      m.gradientFloats == std::vector<float>{0,1,0,0,0.5F,0.5F,0.25F,0.125F,1,0,0,1},
                  "literal ordered packed RGB values");
    unsigned paintProperties = 0;
    for (const auto &p : m.properties) if (p.owner == n.id) {
        ++paintProperties;
        vectorRequire(p.flags == PropertyFlagStatic && !p.track.valid() &&
                          p.semantic != PropertySemantic::FillColor, "gradient genuinely static properties");
        if (p.semantic == PropertySemantic::GradientStops)
            vectorRequire(p.valueType == PropertyValueType::Gradient && p.staticValue.index == 0,
                          "gradient static value reference");
    }
    vectorRequire(paintProperties == 4, "start/end/opacity/stops bindings only");
    unsigned rejected = 0;
    const auto reject = [&](const std::string &json, const std::string &label) {
        const auto r = vectorCompile(json);
        vectorRequire(!r && !r.prepared && r.path.starts_with("/layers/") && !r.message.empty(), "gradient rejection: " + label);
        ++rejected;
    };
    const auto fill = gradientFill();
    for (const auto &p : {"1", "17", "2.5", "true", "\"3\""})
        reject(gradientFixture(vectorReplace(fill, "\"p\":3", "\"p\":" + std::string(p))), "stop count " + std::string(p));
    for (const auto &t : {"0", "2", "3", "1.5", "false"})
        reject(gradientFixture(vectorReplace(fill, "\"t\":1", "\"t\":" + std::string(t))), "type " + std::string(t));
    const std::string packed = "[0,1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]";
    for (const auto &k : {"[0,1,0,0,1,0,0,1]", "[0,1,0,0,0.5,0.5,0.25,0.125,1,0,0,1,0,1]",
                          "[{\"t\":0,\"s\":[0,1,0,0]}]", "[0,1,0,0,0,0.5,0.25,0.125,1,0,0,1]",
                          "[0,1,0,0,0.8,0.5,0.25,0.125,0.7,0,0,1]", "[-0.1,1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]",
                          "[0,1.1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]", "[0,true,0,0,0.5,0.5,0.25,0.125,1,0,0,1]"})
        reject(gradientFixture(vectorReplace(fill, packed, k)), "packed mismatch/order/range/type");
    for (const auto &a : {"1", "false", "\"0\"", "0.1"}) {
        auto bad = vectorReplace(fill, "\"g\":{\"p\":3,\"k\":{\"a\":0", "\"g\":{\"p\":3,\"k\":{\"a\":" + std::string(a));
        reject(gradientFixture(bad), "stops animation flag");
        for (const auto &field : {"s", "e", "o"}) {
            const std::string prefix = "\"" + std::string(field) + "\":{\"a\":0";
            reject(gradientFixture(vectorReplace(fill,prefix,"\"" + std::string(field) + "\":{\"a\":" + a)),
                   "endpoint/opacity animation flag");
        }
    }
    for (const auto &change : {
             std::pair{std::string("\"ty\":\"gf\""), std::string("\"ty\":\"gs\"")},
             {"\"t\":1", "\"t\":1,\"unknown\":0"}, {"\"p\":3", "\"p\":3,\"unknown\":0"},
             {"\"k\":" + packed, "\"unknown\":0,\"k\":" + packed},
             {"\"t\":1", "\"t\":1,\"hd\":true"}, {"\"t\":1", "\"t\":1,\"bm\":1"},
             {"\"t\":1", "\"t\":1,\"h\":{\"a\":0,\"k\":1}"},
             {"\"t\":1", "\"t\":1,\"a\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[0]},{\"t\":10,\"s\":[0]}]}"},
             {"[18,3]", "[2,3]"}, {"[18,3]", "[32769,3]"}, {"[18,3]", "[18,3,0]"},
             {"\"o\":{\"a\":0,\"k\":100}", "\"o\":{\"a\":0,\"k\":true}"},
             {"\"s\":{\"a\":0,\"k\":[2,3]}", "\"s\":{\"a\":0,\"k\":[{\"t\":0,\"s\":[2,3]}]}"}})
        reject(gradientFixture(vectorReplace(fill, change.first, change.second)), "operative/static fields");
    reject(vectorRoot(vectorGroupsLayer(gradientGroup(R"({"ty":"fl","c":{"k":[1,0,0,1]}})") + "," + fill)), "outer gradient");
    reject(gradientFixture(vectorReplace(fill,"\"ty\":\"gf\"","\"ty\":\"gf\",\"a\":{\"a\":0,\"k\":1}")),
           "nonzero highlight angle");
    reject(gradientFixture(vectorReplace(fill,"\"t\":1,","")),"required linear type");
    reject(gradientFixture(vectorReplace(fill,"\"p\":3,","")),"required stop count");
    reject(gradientFixture(vectorReplace(fill,"\"g\":{\"p\":3","\"g\":{\"p\":3,\"unknown\":true")),"unknown gradient fields");
    reject(gradientFixture(vectorReplace(fill,"\"s\":{\"a\":0","\"s\":{\"unknown\":0,\"a\":0")),"unknown endpoint property field");
    reject(gradientFixture(vectorReplace(fill,"\"g\":{\"p\":3,\"k\":{\"a\":0",
        "\"g\":{\"p\":3,\"k\":{\"ix\":false,\"a\":0")),"invalid stop property index");
    reject(gradientFixture(vectorReplace(fill,packed,"[0,-0.1,0,0,0.5,0.5,0.25,0.125,1,0,0,1]")),"negative RGB channel");
    reject(gradientFixture(vectorReplace(fill,packed,"[0,1,0,0,0.5,0.5,0.25,0.125,1.1,0,0,1]")),"offset above one");
    reject(gradientFixture(vectorReplace(fill,"[18,3]","[2.00000000000000001,3]")),"coincident converted endpoints");
    reject(gradientFixture(vectorReplace(fill,"\"o\":{\"a\":0,\"k\":100}",
        "\"o\":{\"a\":0,\"k\":[{\"t\":0,\"s\":[100]}]}")),"keyed opacity despite a0");
    reject(vectorRoot(vectorGroupsLayer(gradientGroup(fill + "," + R"({"ty":"fl","c":{"k":[1,0,0,1]}})"))),"one-fill ordering");
    const auto neutral = vectorCompile(gradientFixture(vectorReplace(fill,"\"t\":1,\"r\":1,\"o\":{\"a\":0,\"k\":100},",
        "\"t\":1,\"h\":{\"k\":0},\"a\":{\"k\":0},")));
    vectorRequire(bool(neutral), "absent rule/opacity defaults and neutral highlights admitted");
    std::string groups;
    for (unsigned i = 0; i < 64; ++i) { if (i) groups += ","; groups += gradientGroup(gradientPacked(16)); }
    const auto boundary = vectorCompile(vectorRoot(vectorGroupsLayer(groups)));
    vectorRequire(bool(boundary) && boundary.prepared->model->gradientFloats.size() == 4096,
                  "4096 gradient float boundary admitted");
    reject(vectorRoot(vectorGroupsLayer(groups + "," + gradientGroup(gradientPacked(2)))), "aggregate float cap before append");
    vectorRequire(bool(vectorCompile(gradientFixture(R"({"ty":"fl","c":{"k":[1,0,0,1]}})"))), "solid remains admissible");
    std::cout << "owned static gradient: packed/statics/defaults; contextual rejections=" << rejected
              << "; aggregate boundary4096/overflow verified\n";
}
int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            gradientModel();
            unsigned failures = 0;
            for (const auto& [label,test] : std::vector<std::pair<std::string,void(*)()>>{
                    {"exporter metadata",exporterMetadata},{"root markers",exporterMarkerAdmission},{"separated position",separatedPosition}}) {
                try { test(); std::cout << "PASS " << label << '\n'; }
                catch(const std::exception& e) { ++failures; std::cerr << "FAIL " << label << ": " << e.what() << '\n'; }
            }
            vectorRequire(!failures,"metadata/position contracts failed=" + std::to_string(failures));
        }
        if (argc == 2 && std::string(argv[1]) == "--group-opacity") {
            groupOpacity();
            return 0;
        }
        float numeric = 0;
        vectorRequire(
            runtime::detail::convertOwnNumericToken("12e1", 0, 240, true, false, numeric) &&
                numeric == 120,
            "existing exact numeric conversion reused");
        for (const auto& token :
             {"1.00000000000000000001", "240.00000000000000000001", "1e999999999999999999999",
              "1e-999999999999999999999", "+1", "1e", "nan", ""})
            vectorRequire(
                !runtime::detail::convertOwnNumericToken(token, 0, 240, true, false, numeric),
                "invalid/exact out of domain numeric token");
        auto json = test::vectorInput(argc, argv);
        exactAdmission();
        auto result = test::vectorCompile(json);
        vectorRequire(bool(result), "vector admission: " + result.path + " " + result.message);
        vectorRequire(runtime::detail::validateOwnVectorModel(*result.prepared),
                      "validated immutable vector owner");
        evaluation::PropertyEvaluator evaluator(result.prepared->model);
        evaluation::PropertyEvaluationWorkspace workspace;
        evaluator.prepare(workspace);
        auto view = evaluator.evaluate(0, workspace);
        vectorRequire(bool(view), "canonical evaluation");
        if (argc == 1) {
            const auto& draws = result.prepared->draws;
            vectorRequire(draws.size() == 2, "two authored paints");
            const auto& transform = view.nodeTransforms[draws[0].layer.index()];
            vectorRequire(transform.worldOpacity == 1, "null parent opacity does not hide child");
            near(transform.worldMatrix.dx, 46, "position rotation scale anchor then parent X");
            near(transform.worldMatrix.dy, 58, "position rotation scale anchor then parent Y");
            const auto& model = *result.prepared->model;
            vectorRequire(model.sourceNodes[draws[0].paint.index()].kind == SourceNodeKind::Fill &&
                              model.sourceNodes[draws[1].paint.index()].kind ==
                                  SourceNodeKind::Stroke,
                          "reverse authored paint order");
            vectorRequire(draws[0].trim && draws[1].trim == draws[0].trim &&
                              draws[0].path == draws[1].path,
                          "outer trim binds nested single path");
            vectorRequire(result.prepared->layers[0].inFrame == 0 &&
                              result.prepared->layers[0].outFrame == 180,
                          "visibility intersects composition not transform parent");
            vectorRequire(model.shapePoints.size() == 4 &&
                              model.shapePoints[1] == MotionVec2Value{2, 0} &&
                              model.shapePoints[2] == MotionVec2Value{8, 0},
                          "relative cubic controls materialized");
            vectorRequire(model.sourceNodes[draws[1].paint.index()].miterLimit == 4,
                          "Lottie default miter limit retained");
            auto omitted = test::vectorCompile(
                vectorReplace(vectorReplace(json, "\"ddd\":0,", ""), "\"ddd\":0,", ""));
            vectorRequire(bool(omitted), "omitted ddd defaults to admitted 2D");
            properties();
            boundedGroups();
            groupOpacity();
            measuredNumericAdmission();
            measuredScopeRejections();
            morphsAndDefaults();
            rejects(json);
        } else {
            const auto& m = *result.prepared->model;
            const auto count = [&](SourceNodeKind kind) {
                return std::count_if(m.sourceNodes.begin(), m.sourceNodes.end(),
                                     [&](const auto& n) { return n.kind == kind; });
            };
            const bool second = std::filesystem::path(argv[2]).filename() == "a-2.tgs";
            if (second)
                vectorRequire(result.prepared->layers.size() == 50 &&
                                  result.prepared->draws.size() == 52 &&
                                  count(SourceNodeKind::Shape) == 38 &&
                                  count(SourceNodeKind::Trim) == 14 &&
                                  m.tracks.size() == 94,
                              "unchanged Duck X3 expanded inventory");
            else
                vectorRequire(
                    result.prepared->layers.size() == 37 && result.prepared->draws.size() == 36 &&
                        count(SourceNodeKind::Shape) == 32 && count(SourceNodeKind::Trim) == 7 &&
                        m.tracks.size() == 33 && m.segments.size() == 304,
                    "unchanged Duck authored inventory");
        }
        std::cout << "PASS own vector layers=" << result.prepared->layers.size()
                  << " draws=" << result.prepared->draws.size()
                  << " tracks=" << result.prepared->model->tracks.size() << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
