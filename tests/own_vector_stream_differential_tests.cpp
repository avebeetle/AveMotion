#include "OwnNativeEllipseStream.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorInstanceTestData.hpp"
#include "support/OwnVectorSceneComparison.hpp"
#include <chrono>
#include <iostream>
using namespace avemotion;
using namespace avemotion::test;
template <class F> void rejects(F operation, const std::string &reason) {
    const auto saved = vector_scene::maxima;
    bool failed = false;
    try {
        operation();
    } catch (const std::runtime_error &) {
        failed = true;
    }
    vector_scene::maxima = saved;
    vectorRequire(failed, "mutation undetected: " + reason);
}
void arithmeticWitnesses() {
    for (const auto scale : {1.0F, 2.0F, 0.25F}) {
        VMatrix m;
        m.translate(32, 64).scale(scale, scale);
        bool invertible = false;
        const auto residual = m * m.inverted(&invertible);
        vectorRequire(invertible, "scale witness invertible");
        const auto p = residual.map(7, 11);
        vectorRequire(p.x() == 7 && p.y() == 11, "translation/scale roundtrip literal");
    }
    VMatrix identity;
    const auto identityPoint = (identity * identity.inverted()).map(7, 11);
    vectorRequire(identityPoint.x() == 7 && identityPoint.y() == 11, "identity roundtrip literal");
    VMatrix rotation;
    rotation.rotate(90);
    const auto rotated = rotation.map(2, 3);
    vector_scene::near(-3, rotated.x(), "arithmetic", "quarter-turn X");
    vector_scene::near(2, rotated.y(), "arithmetic", "quarter-turn Y");
    for (const auto shift : {0.0000001F, 0.00001F}) {
        VMatrix m;
        m.translate(shift, 0);
        const auto p = m.map(0, 0);
        vectorRequire(p.x() == (shift == 0.0000001F ? 0.0F : 0.00001F),
                      "pinned translate type boundary");
        const auto r = m * m.inverted();
        const auto roundtrip = r.map(7, 11);
        vectorRequire(roundtrip.x() == 7 && roundtrip.y() == 11, "type boundary roundtrip");
    }
}
std::string mixedComposition() {
    auto child = vectorShapeLayer("{\"k\":" + vectorShape(3, true) + "}");
    child = vectorReplace(child, "{\"ty\":\"tr\"}",
                          "{\"ty\":\"fl\",\"c\":{\"k\":[0,1,0,1]}},{\"ty\":\"tr\"}");
    auto top = vectorReplace(child, "[0,1,0,1]", "[1,0,0,1]");
    top = vectorReplace(top, "\"ind\":1", "\"ind\":2");
    auto bottom = vectorReplace(child, "[0,1,0,1]", "[0,0,1,1]");
    bottom = vectorReplace(bottom, "\"ind\":1", "\"ind\":3");
    auto json = vectorPrecomp(child);
    json = vectorReplace(json, "\"fr\":60", "\"v\":\"5.5.2\",\"fr\":60");
    json = vectorReplace(json, "\"layers\":[{\"ddd\":0,\"ty\":0",
                         "\"layers\":[" + top + ",{\"ddd\":0,\"ty\":0");
    return vectorReplace(json, "\"ks\":{}}]}", "\"ks\":{}}," + bottom + "]}");
}
void run(const std::string &json, bool rectangles = false, bool exhaustive = false) {
    vector_scene::permittedInactiveMiter.clear();
    vector_scene::observedInactiveMiter.clear();
    const auto compiled = vectorCompile(json);
    vectorRequire(bool(compiled), compiled.path + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    vectorRequire(bool(prepared), prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    vectorRequire(bool(stream), stream.message);
    OwnVectorSceneOracle oracle(json);
    vector_scene::OwnRoles ownRoles(*prepared.prepared);
    {
        auto own = stream.stream->emit(3, 512, 512);
        auto ref = oracle.freshScene(3, 512, 512);
        vectorRequire(bool(own), own.message);
        const auto full = [&](const runtime::EvaluatedScene &changed) {
            vector_scene::compare(ref, oracle, changed, ownRoles);
        };
        full(*own.scene);
        auto changed = *own.scene;
        changed.drawItems.front().localPath.points.front().x += 1;
        rejects([&] { full(changed); }, "local-only curve point");
        changed = *own.scene;
        changed.drawItems.front().localPath.controlBounds.left += 1;
        rejects([&] { full(changed); }, "stored bounds only");
        changed = *own.scene;
        changed.drawItems.front().paint.solid.a = 0;
        rejects([&] { full(changed); }, "inherited null-parent opacity");
        if (changed.drawItems.size() > 1) {
            changed = *own.scene;
            std::swap(changed.drawItems[0], changed.drawItems[1]);
            rejects([&] { full(changed); }, "paint/layer reordering");
        }
        changed = *own.scene;
        for (auto &item : changed.drawItems)
            if (item.stroke.enabled && item.stroke.join == runtime::LineJoin::Miter) {
                item.stroke.miterLimit += 1;
                rejects([&] { full(changed); }, "active miter");
                break;
            }
        bool cancellationWitness = false;
        for (std::size_t i = 0; i < ref.drawItems.size(); ++i) {
            const auto &a = ref.drawItems[i];
            const auto &b = own.scene->drawItems[i];
            if (a.localPath.points.empty())
                continue;
            if (std::abs(a.localPath.points[0].y - b.localPath.points[0].y) <= 1e-4)
                continue;
            const auto &m = oracle.matrices().at(a.modelNode.value);
            const auto residual = oracle.sourceWorld(ref, a, a.sourcePathNode) * m.inverted();
            auto local = b.localPath;
            local.points[0].y += 0.01F;
            local.controlBounds = vector_scene::pointBounds(local);
            rejects(
                [&] {
                    vector_scene::compareLocal(a.localPath, local, residual,
                                               "cancellation point mutation");
                },
                "cancellation local-only point");
            local = b.localPath;
            local.controlBounds.top += 0.01F;
            rejects(
                [&] {
                    vector_scene::compareLocal(a.localPath, local, residual,
                                               "cancellation bounds mutation");
                },
                "cancellation bounds-only");
            rejects(
                [&] {
                    vector_scene::compareLocal(a.localPath, b.localPath, VMatrix{},
                                               "wrong identity predictor");
                },
                "cancellation wrong predictor");
            cancellationWitness = true;
            break;
        }
        if (compiled.prepared->layers.size() == 37)
            vectorRequire(cancellationWitness, "real cancellation mutation witness exercised");
        std::cout << "scene mutations passed; cancellation=" << cancellationWitness << '\n';
    }
    vector_scene::maxima.clear();
    vector_scene::rawLocalMaxima.clear();
    vector_scene::rawLocalWorstContext.clear();
    vector_scene::rawLocalOverLimit = 0;
    std::size_t samples = 0;
    const auto compare = [&](std::size_t frame, std::size_t width, std::size_t height) {
        auto own = stream.stream->emit(frame, width, height);
        vectorRequire(bool(own), own.message);
        auto ref = oracle.freshScene(frame, width, height);
        vector_scene::compare(ref, oracle, *own.scene, ownRoles);
        ++samples;
    };
    for (auto width :
         exhaustive ? std::vector<unsigned>{128, 129, 256, 512} : std::vector<unsigned>{512}) {
        for (std::size_t f = 0; f < 180; ++f)
            compare(f, width, width);
        for (std::size_t f = 180; f-- > 0;)
            compare(f, width, width);
    }
    for (auto f :
         {179U, 0U, 33U, 34U, 67U, 68U, 127U, 128U, 137U, 138U, 90U, 15U, 14U, 15U, 179U, 0U})
        for (auto width : {128U, 129U, 256U, 512U, 128U})
            compare(f, width, width);
    if (exhaustive) {
        auto second = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        vectorRequire(bool(second), second.message);
        for (auto width : {128U, 129U, 256U, 512U})
            for (std::size_t f = 0; f < 180; ++f) {
                const auto other = second.stream->emit(179 - f, width, width);
                vectorRequire(bool(other), other.message);
                auto ref = oracle.freshScene(179 - f, width, width);
                vector_scene::compare(ref, oracle, *other.scene, ownRoles);
                ++samples;
                compare(f, width, width);
            }
    }
    if (rectangles)
        for (std::size_t f = 0; f < 180; ++f) {
            compare(f, 512, 256);
            compare(f, 256, 512);
        }
    std::cout << "full visible scene samples=" << samples
              << " inactive-miter roles=" << vector_scene::observedInactiveMiter.size() << '\n';
    for (const auto &[field, error] : vector_scene::maxima)
        std::cout << field << " max=" << error << '\n';
    std::cout.precision(12);
    for (const auto &[field, error] : vector_scene::rawLocalMaxima)
        std::cout << "RAW " << field << " max=" << error << " at "
                  << vector_scene::rawLocalWorstContext[field] << '\n';
    std::cout << "raw local over-limit fields=" << vector_scene::rawLocalOverLimit << '\n';
    vectorRequire(vector_scene::numericFailures == 0,
                  "numeric threshold violations=" + std::to_string(vector_scene::numericFailures));
}
void authoredMutation(const std::string &original, const std::string &changed,
                      const std::string &label) {
    OwnVectorSceneOracle oracle(original);
    const auto ref = oracle.freshScene(10, 512, 512);
    const auto model = vectorCompile(changed);
    vectorRequire(bool(model), label + " mutation still admitted");
    const auto prepared = render::detail::prepareOwnVectorAsset(model.prepared);
    vectorRequire(bool(prepared), prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    const auto own = stream.stream->emit(10, 512, 512);
    vectorRequire(bool(own), own.message);
    vector_scene::OwnRoles ownRoles(*prepared.prepared);
    rejects([&] { vector_scene::compare(ref, oracle, *own.scene, ownRoles); }, label);
}
std::string repeatedOracleFixture() {
    auto leaf = vectorReplace(
        clippingShape(30, 30, 40, 40), "\"ks\":{}",
        R"("st":-5,"ks":{"p":{"a":1,"k":[{"t":0,"s":[10,0],"h":1},{"t":28.822,"s":[20,0],"h":1},{"t":110,"s":[30,0]}]}})");
    leaf = vectorReplace(leaf, "\"op\":180", "\"op\":110");
    const auto inner = instancePrecomp(21, "inner", 86, 206, 86, R"("p":{"k":[12,9]})") + "," +
                       instancePrecomp(22, "inner", 10, 130, 10, R"("p":{"k":[12,9]})");
    return vectorReplace(
        instanceRoot(instanceAsset("outer", inner) + "," + instanceAsset("inner", leaf),
                     instancePrecomp(1, "outer", 128, 308, 128) + "," +
                         instancePrecomp(2, "outer", -52, 128, -52)),
        "\"fr\":60", "\"v\":\"5.5.2\",\"fr\":60");
}
std::string opacityOracleFixture() {
    auto leaf = vectorReplace(
        clippingShape(30, 30, 40, 40), "\"ks\":{}",
        R"("ks":{"o":{"a":1,"k":[{"t":0,"s":[100],"h":1},{"t":4,"s":[0],"h":1},{"t":8,"s":[100]}]}})");
    return vectorReplace(vectorRoot(leaf), "\"fr\":60", "\"v\":\"5.5.2\",\"fr\":60");
}
void instanceOracleWitnesses() {
    const auto json = repeatedOracleFixture();
    OwnVectorSceneOracle oracle(json);
    const auto compiled = vectorCompile(json);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    vector_scene::OwnRoles roles(*prepared.prepared);
    const auto own = stream.stream->emit(34, 256, 256);
    auto ref = oracle.freshScene(34, 256, 256);
    vector_scene::compare(ref, oracle, *own.scene, roles);
    vectorRequire(ref.drawItems.size() == 2, "two simultaneous reference occurrences");
    const auto &a = ref.drawItems[0], &b = ref.drawItems[1];
    vectorRequire(a.sourcePathNode == b.sourcePathNode && a.sourcePaintNode == b.sourcePaintNode &&
                      a.modelNode != b.modelNode,
                  "shared definition, unique reference execution draw");
    std::set<int> clocks;
    std::set<std::string> paths;
    for (const auto &draw : ref.drawItems) {
        const auto &instance = oracle.instances().at(ref.layers[draw.layerIndex].modelLayer.value);
        clocks.insert(34 - instance.offset);
        paths.insert(instance.path);
    }
    vectorRequire(
        clocks == std::set<int>{0, 76} &&
            paths == std::set<std::string>{"root/layer:2[1]->outer/layer:21[0]->inner/layer:1[0]",
                                           "root/layer:2[1]->outer/layer:22[1]->inner/layer:1[0]"},
        "literal independent instance ancestry and clocks");
    const auto compareOwn = [&](const auto &changed) {
        vector_scene::compare(ref, oracle, changed, roles);
    };
    auto changed = *own.scene;
    changed.drawItems[0].layerIndex = changed.drawItems[1].layerIndex;
    rejects([&] { compareOwn(changed); }, "own draw assigned other repeated layer");
    changed = *own.scene;
    changed.drawItems[0].sourcePathNode = changed.drawItems[1].sourcePathNode;
    rejects([&] { compareOwn(changed); }, "own repeated path source substituted");
    changed = *own.scene;
    std::swap(changed.drawItems[0], changed.drawItems[1]);
    rejects([&] { compareOwn(changed); }, "equal-definition repeated draw order");
    auto referenceMutation = ref;
    referenceMutation.layers.back().modelLayer = referenceMutation.layers.front().modelLayer;
    rejects([&] { oracle.validateInstances(referenceMutation); },
            "reference duplicate execution layer");
    referenceMutation = ref;
    referenceMutation.drawItems[0].modelNode = referenceMutation.drawItems[1].modelNode;
    rejects([&] { oracle.validateInstances(referenceMutation); },
            "reference duplicate execution draw");
    referenceMutation = ref;
    referenceMutation.layers[a.layerIndex].parentLayer = runtime::kInvalidSceneIndex;
    rejects([&] { oracle.validateInstances(referenceMutation); },
            "reference removed structural ancestry");
    referenceMutation = ref;
    referenceMutation.layers[a.layerIndex].visible = false;
    rejects([&] { oracle.validateInstances(referenceMutation); },
            "reference wrong local visibility");
    referenceMutation = ref;
    auto clip = std::find_if(referenceMutation.layers.begin(), referenceMutation.layers.end(),
                             [](const auto &layer) { return !layer.clipPath.points.empty(); });
    vectorRequire(clip != referenceMutation.layers.end(), "translated clip mutation available");
    clip->clipPath.points[0].x += 1;
    rejects([&] { oracle.assertCanvasClip(referenceMutation); }, "reference displaced clip");
    referenceMutation = ref;
    for (auto &layer : referenceMutation.layers)
        if (!layer.clipPath.points.empty()) {
            layer.clipPath = {};
            break;
        }
    rejects([&] { oracle.assertCanvasClip(referenceMutation); }, "reference omitted required clip");
    const auto wrongClock = stream.stream->emit(63, 256, 256);
    rejects([&] { compareOwn(*wrongClock.scene); }, "other local-clock sample");
    std::cout << "instance-role/clock/clip mutation witnesses passed\n";
}
void outerPaintWitness() {
    auto leaf = clippingShape(30, 30, 40, 40);
    leaf = vectorReplace(
        leaf, R"({"ty":"tr"}]}])",
        R"({"ty":"tr","p":{"k":[12,9]},"s":{"k":[75,75]}}]},{"ty":"st","c":{"k":[0,0,1,1]},"w":{"k":4},"lc":1,"lj":1,"ml":4}])");
    const auto json = vectorReplace(vectorRoot(leaf), "\"fr\":60", "\"v\":\"5.5.2\",\"fr\":60");
    const auto compiled = vectorCompile(json);
    vectorRequire(bool(compiled), compiled.path + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    const auto own = stream.stream->emit(3, 128, 128);
    OwnVectorSceneOracle oracle(json);
    const auto ref = oracle.freshScene(3, 128, 128);
    vector_scene::OwnRoles roles(*prepared.prepared);
    vector_scene::compare(ref, oracle, *own.scene, roles);
    bool witnessed = false;
    for (std::size_t i = 0; i < ref.drawItems.size(); ++i) {
        const auto &draw = ref.drawItems[i];
        if (!draw.stroke.enabled)
            continue;
        const auto m = oracle.matrices().at(draw.modelNode.value);
        vectorRequire(draw.localPath.points.front().x == 34.5F &&
                          own.scene->drawItems[i].localPath.points.front().x == 30,
                      "outer stroke raw local spaces differ by literal group transform");
        rejects(
            [&] {
                vector_scene::compareLocal(draw.localPath, own.scene->drawItems[i].localPath,
                                           m * m.inverted(), "wrong same-group predictor");
            },
            "outer paint requires group-world times inverse paint-world");
        auto wrong = *own.scene;
        wrong.drawItems[i].localToViewport = draw.localToViewport;
        rejects([&] { vector_scene::compare(ref, oracle, wrong, roles); },
                "outer draw assigned paint-world instead of path-world");
        witnessed = true;
    }
    vectorRequire(witnessed, "outer stroke mutation exercised");
    std::cout << "outer paint transform mutation witnesses passed\n";
}
void strokeScaleWitness() {
    auto leaf = vectorReplace(clippingShape(30, 30, 40, 40, true), "\"ks\":{}",
                              R"("ks":{"p":{"k":[160,280]}})");
    const auto json = vectorReplace(vectorRoot(leaf), "\"fr\":60", "\"v\":\"5.5.2\",\"fr\":60");
    const auto compiled = vectorCompile(json);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    const auto own = stream.stream->emit(0, 128, 128);
    OwnVectorSceneOracle oracle(json);
    const auto ref = oracle.freshScene(0, 128, 128);
    std::cout.precision(12);
    std::cout << "translated stroke scale own=" << own.scene->drawItems[0].stroke.width
              << " reference=" << ref.drawItems[0].stroke.width << '\n';
    vectorRequire(own.scene->drawItems[0].stroke.width == ref.drawItems[0].stroke.width,
                  "translated stroke preserves pinned mapped-diagonal cancellation");
}
void costs(const std::string &json) {
    using Clock = std::chrono::steady_clock;
    const auto micros = [](auto first, auto last) {
        return std::chrono::duration<double, std::micro>(last - first).count();
    };
    std::cout.precision(12);
    std::cout << "cost route=own decoded_bytes=" << json.size() << " decoded_fnv1a64="
              << core::fnv1a64(std::as_bytes(std::span(json.data(), json.size())))
              << " iterations=5 frames_per_iteration=720\n";
    for (unsigned iteration = 0; iteration < 5; ++iteration) {
        const auto start = Clock::now();
        const auto read = formats::detail::readOwnJson(json, {65536, 32});
        const auto parsed = Clock::now();
        vectorRequire(bool(read), "cost parse");
        const auto compiled = runtime::detail::buildOwnVectorModel(*read.document);
        const auto built = Clock::now();
        vectorRequire(bool(compiled), compiled.message);
        const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        const auto ready = Clock::now();
        vectorRequire(bool(prepared), prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        const auto created = Clock::now();
        vectorRequire(bool(stream), stream.message);
        std::size_t draws = 0, points = 0, changed = 0;
        std::uint64_t sequence = 0;
        for (unsigned width : {128U, 129U, 256U, 512U})
            for (unsigned frame = 0; frame < 180; ++frame) {
                const auto sample = stream.stream->emit(frame, width, width);
                vectorRequire(bool(sample), sample.message);
                draws += sample.scene->drawItems.size();
                points += sample.scene->statistics.pathPointCount;
                changed += sample.scene->changes.visualChanged;
                sequence = sample.scene->evaluationSequence;
            }
        const auto replayed = Clock::now();
        std::cout << "cost iteration=" << iteration << " read_us=" << micros(start, parsed)
                  << " compile_us=" << micros(parsed, built)
                  << " prepare_us=" << micros(built, ready)
                  << " create_us=" << micros(ready, created)
                  << " replay720_us=" << micros(created, replayed)
                  << " json_values=" << read.statistics.nodeCount
                  << " layers=" << compiled.prepared->layers.size()
                  << " properties=" << compiled.prepared->model->properties.size()
                  << " draws=" << draws << " path_points=" << points
                  << " changed_frames=" << changed << " sequence=" << sequence << '\n';
        if (iteration == 0) {
            evaluation::PropertyEvaluator evaluator(prepared.prepared->model,
                                                    compiled.prepared->propertyFrameOffsets);
            evaluation::PropertyEvaluationWorkspace workspace;
            evaluator.prepare(workspace);
            std::size_t visits = 0, cursor = 0, adjacent = 0, search = 0, shapes = 0;
            for (unsigned frame = 0; frame < 180; ++frame) {
                const auto view = evaluator.evaluate(frame, workspace);
                vectorRequire(bool(view), "cost evaluator");
                visits += view.statistics.propertiesVisited;
                cursor += view.statistics.cursorHits;
                adjacent += view.statistics.adjacentCursorMoves;
                search += view.statistics.binarySearches;
                shapes += view.statistics.shapePointInterpolations;
            }
            std::cout << "separate retained evaluator180 counters properties_visited=" << visits
                      << " cursor_hits=" << cursor << " adjacent_moves=" << adjacent
                      << " binary_searches=" << search << " shape_point_interpolations=" << shapes
                      << '\n';
        }
    }
}
int main(int argc, char **argv) {
    try {
        if (argc == 4 && std::string(argv[3]) == "--costs") {
            costs(vectorInput(3, argv));
            return 0;
        }
        const bool exhaustive = argc == 4 && std::string(argv[3]) == "--full";
        if (exhaustive)
            --argc;
        if (argc == 2 && std::string(argv[1]) == "--instances") {
            instanceOracleWitnesses();
            outerPaintWitness();
            run(repeatedOracleFixture());
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--stroke-scale") {
            strokeScaleWitness();
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--opacity") {
            run(opacityOracleFixture());
            return 0;
        }
        if (argc == 4 && std::string(argv[3]) == "--diagnostic") {
            vector_scene::collectNumericFailures = true;
            --argc;
        }
        arithmeticWitnesses();
        const auto input = vectorInput(argc, argv);
        // Direct real-Duck rectangle admission is conservatively bounded;
        // rectangular rejection probes live in the stream lifecycle test.
        run(input, false, exhaustive);
        if (argc == 1) {
            instanceOracleWitnesses();
            outerPaintWitness();
            strokeScaleWitness();
            run(repeatedOracleFixture());
            run(opacityOracleFixture());
            const auto animated = vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) /
                                             "own_vector/animated.json");
            run(animated);
            run(mixedComposition());
            run(clippingPrecomp(clippingShape(56, 56, 72, 72, true)), true);
            authoredMutation(input,
                             vectorReplace(input,
                                           ",{\"ty\":\"tm\",\"s\":{\"a\":0,\"k\":0},\"e\":{\"a\":"
                                           "0,\"k\":95},\"o\":{\"a\":0,\"k\":0},\"m\":1}",
                                           ""),
                             "removed trim");
            authoredMutation(
                animated,
                vectorReplace(animated, "\"t\":20,\"s\":[10,0,0]", "\"t\":20,\"s\":[20,0,0]"),
                "changed spatial curve");
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
