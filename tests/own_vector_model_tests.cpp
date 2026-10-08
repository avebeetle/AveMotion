#include "NativeEllipseAdmissionCore.hpp"
#include "OwnVectorTestData.hpp"
#include "support/OwnVectorGroupTestData.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
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
int main(int argc, char** argv) {
    try {
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
