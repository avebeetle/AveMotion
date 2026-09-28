#pragma once
#include "OwnVectorClippingTestData.hpp"
#include "OwnNativeEllipseStream.hpp"
#include <algorithm>
#include <iostream>
#include <set>

namespace avemotion::test {
inline std::string instancePrecomp(int id, const std::string& asset, int first = 0,
                                  int end = 180, int start = 0,
                                  const std::string& transform = "", int width = 128) {
    return "{\"ty\":0,\"ind\":" + std::to_string(id) + ",\"refId\":\"" + asset +
           "\",\"w\":" + std::to_string(width) + ",\"h\":128,\"ip\":" +
           std::to_string(first) + ",\"op\":" + std::to_string(end) + ",\"st\":" +
           std::to_string(start) + ",\"ks\":{" + transform + "}}";
}
inline std::string instanceAsset(const std::string& id, const std::string& layers) {
    return "{\"id\":\"" + id + "\",\"layers\":[" + layers + "]}";
}
inline std::string instanceRoot(const std::string& assets, const std::string& layers) {
    return vectorReplace(vectorRoot(layers), "\"layers\":", "\"assets\":[" + assets + "],\"layers\":");
}
inline auto instanceStream(const std::string& json) {
    const auto compiled = vectorCompile(json);
    vectorRequire(bool(compiled), "instance compile: " + compiled.path + " " + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    vectorRequire(bool(prepared), "instance prepare: " + prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    vectorRequire(bool(stream), stream.message);
    return std::move(stream.stream);
}
inline void repeatedInstanceClocks() {
    // Catch shared clocks, shifting leaf st, rebased fractional keys, visibility
    // off-by-one errors, and cursors that retain another occurrence's sample.
    auto leaf = vectorReplace(clippingShape(30, 30, 40, 40), "\"ks\":{}",
        R"("st":-5,"ks":{"p":{"a":1,"k":[{"t":0,"s":[10,0],"h":1},{"t":28.822,"s":[20,0],"h":1},{"t":110,"s":[30,0]}]}})");
    leaf = vectorReplace(leaf, "\"op\":180", "\"op\":110");
    const auto inner = instancePrecomp(21, "inner", 86, 206, 86, R"("p":{"k":[12,9]})") + "," +
                       instancePrecomp(22, "inner", 10, 130, 10, R"("p":{"k":[12,9]})");
    const auto json = instanceRoot(instanceAsset("outer", inner) + "," + instanceAsset("inner", leaf),
        instancePrecomp(1, "outer", 128, 308, 128) + "," + instancePrecomp(2, "outer", -52, 128, -52));
    auto stream = instanceStream(json);
    const auto authored = vectorCompile(json);
    std::set<std::int32_t> leafOffsets;
    for (const auto& source : authored.prepared->sources) {
        if (source.jsonPointer != "/assets/1/layers/0") continue;
        const auto& node = authored.prepared->model->sourceNodes[source.source.index()];
        vectorRequire(node.startFrame == -5 && node.inFrame == 0 && node.outFrame == 110 && node.timeStretch == 1,
                      "authored leaf clock metadata retained");
        leafOffsets.insert(source.frameOffset);
    }
    vectorRequire(leafOffsets == std::set<std::int32_t>{-42,34,138,214},
                  "leaf clock offsets preserve composition ancestry");
    for (const auto& track : authored.prepared->model->tracks) {
        const auto& segment = authored.prepared->model->segments[track.segments.first+1];
        vectorRequire(segment.firstFrame == static_cast<double>(28.822F),
                      "authored fractional keys are never rebased");
    }
    const std::vector<std::pair<unsigned, std::vector<float>>> cases{
        {0,{32}}, {33,{32}}, {34,{22,32}}, {62,{22,32}}, {63,{32,32}},
        {67,{32,32}}, {68,{32}}, {127,{32}}, {128,{}}, {137,{}}, {138,{22}},
        {179,{32}}, {34,{22,32}}, {138,{22}}, {0,{32}}};
    for (const auto& [frame, expected] : cases) {
        const auto scene = stream->emit(frame, 128, 128);
        vectorRequire(bool(scene), scene.message);
        std::vector<float> positions;
        std::set<std::uint32_t> identities;
        for (const auto& draw : scene.scene->drawItems) {
            positions.push_back(draw.localToViewport.dx);
            identities.insert(draw.modelNode.value);
        }
        std::sort(positions.begin(), positions.end());
        vectorRequire(positions == expected && identities.size() == positions.size(),
                      "literal nested clocks/visibility at " + std::to_string(frame));
    }
    auto independent = instanceStream(json);
    const auto other = independent->emit(34,256,256);
    vectorRequire(bool(other) && other.scene->changes.firstEvaluation && other.scene->evaluationSequence == 1 &&
                  other.scene->drawItems.size() == 2 && other.scene->drawItems[0].localToViewport.dx == 64 &&
                  other.scene->drawItems[1].localToViewport.dx == 44,
                  "independent stream and viewport retain separate local clocks");
    auto marker = vectorReplace(clippingShape(30,30,40,40), "\"ks\":{}",
        R"("ks":{"p":{"a":1,"k":[{"t":0,"s":[0,0],"h":1},{"t":179,"s":[30,0]}]}})");
    auto splice = instanceStream(instanceRoot(instanceAsset("x", marker),
        instancePrecomp(1,"x",128,308,128) + "," + instancePrecomp(2,"x",-52,128,-52)));
    for (const auto [frame, want] : {std::pair{127U,30.F}, {128U,0.F}, {127U,30.F}}) {
        const auto scene = splice->emit(frame,128,128);
        vectorRequire(bool(scene) && scene.scene->drawItems.size() == 1 &&
                      scene.scene->drawItems[0].localToViewport.dx == want,
                      "root splice local179/local0");
    }
}
inline void translatedInstanceClips() {
    // Catch canvas-only clip waivers, missing ancestor clips, sibling clip leaks,
    // insufficient stroke padding, and publishing a failed scene into history.
    const auto nested = [](const std::string& leaf, int outerWidth = 128) {
        return instanceRoot(instanceAsset("outer", instancePrecomp(1,"inner",0,180,0,
                            R"("p":{"k":[12,9]})")) + "," + instanceAsset("inner",leaf),
                            instancePrecomp(1,"outer",0,180,0,"",outerWidth));
    };
    for (const auto& [leaf, supported] : std::vector<std::pair<std::string,bool>>{
        {clippingShape(30,30,40,40),true}, {clippingShape(-8,30,40,40),false},
        {clippingShape(30,-8,40,40),false}, {clippingShape(6,30,40,40),true},
        {clippingShape(6,30,40,40,true),false},
        {clippingShape(200,30,220,40),true}}) {
        auto stream = instanceStream(nested(leaf));
        const auto result = stream->emit(0,128,128);
        vectorRequire(bool(result) == supported && (supported ||
                      (!result.scene && result.code == render::detail::OwnNativeEllipseFrameCode::UnsupportedClipping)),
                      "translated inner raster containment");
    }
    auto ancestor = instanceStream(nested(clippingShape(60,30,80,40),64));
    vectorRequire(!ancestor->emit(0,128,128), "outer clip remains enforced");
    const auto rootLeaf = vectorReplace(clippingShape(0,0,8,8), "\"ind\":1", "\"ind\":2");
    auto sibling = instanceStream(instanceRoot(instanceAsset("x",clippingShape(30,30,40,40)),
        instancePrecomp(1,"x",0,180,0,R"("p":{"k":[12,9]})") + "," + rootLeaf));
    vectorRequire(bool(sibling->emit(0,128,128)), "clip does not leak to root sibling");
    auto siblingPrecomp = instanceStream(instanceRoot(instanceAsset("x",clippingShape(30,30,40,40)) + "," +
        instanceAsset("y",clippingShape(0,0,8,8)), instancePrecomp(1,"y") + "," +
        instancePrecomp(2,"x",0,180,0,R"("p":{"k":[12,9]})")));
    vectorRequire(bool(siblingPrecomp->emit(0,128,128)), "translated clips do not leak across precomp siblings");
    auto empty = clippingShape(-8,-8,40,40);
    empty = vectorReplace(empty, "{\"ty\":\"tr\"}]}]", R"({"ty":"tr"}]},{"ty":"tm","m":1,"s":{"k":50},"e":{"k":50}}])");
    auto emptyStream = instanceStream(nested(empty));
    const auto emptyScene = emptyStream->emit(0,128,128);
    vectorRequire(bool(emptyScene) && emptyScene.scene->drawItems.size() == 1 &&
                  emptyScene.scene->drawItems[0].path.points.empty(), "empty path needs no clip coverage");
    auto moving = vectorReplace(clippingShape(30,30,40,40), "\"ks\":{}",
        R"("ks":{"p":{"a":1,"k":[{"t":0,"s":[0,0],"h":1},{"t":10,"s":[-40,0]}]}})");
    auto recovery = instanceStream(nested(moving));
    const auto first = recovery->emit(0,128,128), rejected = recovery->emit(10,128,128),
               again = recovery->emit(0,128,128);
    vectorRequire(bool(first) && !rejected && !rejected.scene && bool(again) &&
                  again.scene->evaluationSequence == 3 && !again.scene->changes.visualChanged &&
                  !again.scene->changes.firstEvaluation &&
                  again.scene->fingerprints.scene == first.scene->fingerprints.scene,
                  "clip rejection preserves successful history");
}
inline void instanceAdmissionAndParents() {
    const auto leaf = vectorLayer("");
    const auto basic = instanceRoot(instanceAsset("x",leaf),instancePrecomp(1,"x"));
    const auto reject = [](const std::string& json, const std::string& reason) {
        const auto result = vectorCompile(json);
        vectorRequire(!result && !result.prepared && !result.path.empty() && !result.message.empty(),reason);
    };
    for (const auto& [from,to] : std::vector<std::pair<std::string,std::string>>{
            {"\"st\":0","\"st\":0.5"}, {"\"st\":0","\"st\":20001"},
            {"\"ip\":0","\"ip\":-20001"}, {"\"op\":180","\"op\":0"},
            {"\"refId\":\"x\"","\"refId\":\"missing\""},
            {"\"st\":0","\"st\":0,\"sr\":0.5"}, {"\"st\":0","\"st\":0,\"parent\":1"},
            {"\"st\":0","\"st\":0,\"tm\":{\"k\":0}"},
            {"\"refId\":\"x\",\"w\":128","\"refId\":\"x\",\"w\":0"},
            {"\"refId\":\"x\",\"w\":128","\"refId\":\"x\",\"w\":4097"},
            {"\"refId\":\"x\",\"w\":128","\"refId\":\"x\",\"w\":12.5"}})
        reject(vectorReplace(basic,from,to),"malformed precomp contract " + to);
    for (const auto& transform : {R"("s":{"k":[100.000001,100]})", R"("r":{"k":0.000001})",
             R"("o":{"k":99.9999999})", R"("p":{"a":1,"k":[{"t":0,"s":[0,0],"h":1},{"t":10,"s":[0,0]}]})"})
        reject(instanceRoot(instanceAsset("x",leaf),instancePrecomp(1,"x",0,180,0,transform)),
               "precomp nontranslation/opacity/animation");
    reject(instanceRoot(instanceAsset("x",leaf)+","+instanceAsset("x",leaf),instancePrecomp(1,"x")),"duplicate assets");
    reject(instanceRoot(instanceAsset("x",leaf)+","+instanceAsset("unused",leaf),instancePrecomp(1,"x")),"unused asset");
    reject(instanceRoot(instanceAsset("x",instancePrecomp(1,"y"))+","+instanceAsset("y",instancePrecomp(1,"x")),
                        instancePrecomp(1,"x")),"indirect asset cycle");
    const auto chain = [&](unsigned depth, int start) {
        std::string assets;
        for (unsigned i = 0; i < depth; ++i) {
            if (i) assets += ",";
            assets += instanceAsset(std::to_string(i),i+1 == depth ? leaf :
                instancePrecomp(1,std::to_string(i+1),-20000,20000,start));
        }
        return instanceRoot(assets,instancePrecomp(1,"0",-20000,20000,start));
    };
    for (int start : {-20000,20000}) {
        const auto valid = vectorCompile(chain(4,start));
        vectorRequire(bool(valid) && valid.prepared->propertyFrameOffsets.back() == start*4,
                      "four levels and inclusive accumulated offset boundary");
        reject(chain(5,start),"fifth precomp nesting level");
    }
    std::string assets, roots;
    for (unsigned i = 0; i < 9; ++i) {
        if (i) { assets += ","; roots += ","; }
        assets += instanceAsset(std::to_string(i),leaf);
        roots += instancePrecomp(static_cast<int>(i+1),std::to_string(i));
        if (i == 7) vectorRequire(bool(vectorCompile(instanceRoot(assets,roots))),"eight assets accepted");
    }
    reject(instanceRoot(assets,roots),"ninth asset rejected");
    std::string many;
    for (unsigned i = 0; i < 64; ++i) {
        if (i) many += ",";
        many += vectorReplace(leaf,"\"ind\":1","\"ind\":"+std::to_string(i+1));
        const auto json = instanceRoot(instanceAsset("x",many),instancePrecomp(1,"x")+","+
                                      instancePrecomp(2,"x",190,200));
        if (i == 62) vectorRequire(bool(vectorCompile(json)),"128 expanded layers including inactive branches");
        if (i == 63) reject(json,"expanded count rejects inactive overflow");
    }
    // Forward sibling parent IDs repeat across occurrences. The hidden/zero-alpha
    // parent still supplies its matrix, but never its visibility or opacity.
    const auto shape = vectorReplace(clippingShape(30,30,40,40),"\"ind\":1","\"ind\":1,\"parent\":2");
    auto parent = vectorReplace(vectorLayer(R"("p":{"a":1,"k":[{"t":0,"s":[5,0],"h":1},{"t":10,"s":[15,0]}]},"o":{"k":0})"),
                                "\"ind\":1","\"ind\":2");
    parent = vectorReplace(parent,"\"ip\":0,\"op\":180","\"ip\":190,\"op\":200");
    const auto json = instanceRoot(instanceAsset("x",shape+","+parent),instancePrecomp(1,"x",0,180,0)+","+
                                  instancePrecomp(2,"x",0,180,10));
    auto stream = instanceStream(json);
    const auto scene = stream->emit(15,128,128);
    vectorRequire(bool(scene) && scene.scene->drawItems.size() == 2 &&
                  scene.scene->drawItems[0].localToViewport.dx == 5 &&
                  scene.scene->drawItems[1].localToViewport.dx == 15 &&
                  scene.scene->drawItems[0].separatedOpacity == 1 && scene.scene->drawItems[1].separatedOpacity == 1,
                  "scoped forward inactive parent retains distinct instance matrix");
    reject(vectorReplace(json,"\"ind\":2,\"ip\":190","\"ind\":2,\"parent\":1,\"ip\":190"),"scoped parent cycle");
    reject(vectorReplace(json,"\"parent\":2","\"parent\":9"),"parent cannot resolve outside occurrence");
    const auto compiled = vectorCompile(json);
    std::set<std::string> paths;
    std::set<std::uint32_t> instances;
    for (const auto& source : compiled.prepared->sources) {
        if (source.jsonPointer == "/assets/0/layers/0/shapes/0/it/0") {
            paths.insert(source.jsonPointer);
            instances.insert(source.containingInstance.value);
        }
    }
    vectorRequire(paths.size() == 1 && instances.size() == 2,"source pointers preserve repeated authored provenance");
}
inline void instanceContracts() {
    unsigned failures = 0;
    for (const auto& [label, test] : std::vector<std::pair<std::string,void(*)()>>{
            {"repeated local clocks", repeatedInstanceClocks}, {"translated clips", translatedInstanceClips},
            {"instance admission and parents", instanceAdmissionAndParents}}) {
        try { test(); std::cout << "PASS " << label << '\n'; }
        catch (const std::exception& e) { ++failures; std::cerr << "FAIL " << label << ": " << e.what() << '\n'; }
    }
    vectorRequire(!failures, "instance contract failures=" + std::to_string(failures));
}
} // namespace avemotion::test
