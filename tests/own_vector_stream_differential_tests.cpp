#include "OwnNativeEllipseStream.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorSceneComparison.hpp"
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
void run(const std::string &json, bool rectangles = false) {
    vector_scene::permittedInactiveMiter.clear();
    vector_scene::observedInactiveMiter.clear();
    vector_scene::inspectMiterSource(json);
    const auto compiled = vectorCompile(json);
    vectorRequire(bool(compiled), compiled.path + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    vectorRequire(bool(prepared), prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    vectorRequire(bool(stream), stream.message);
    OwnVectorSceneOracle oracle(json);
    {
        auto own = stream.stream->emit(3, 512, 512);
        auto ref = oracle.freshScene(3, 512, 512);
        vectorRequire(bool(own), own.message);
        const auto full = [&](const runtime::EvaluatedScene &changed) {
            vector_scene::compare(ref, oracle.model(), changed, *prepared.prepared->model,
                                  oracle.matrices());
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
            const auto &m = oracle.matrices().at(a.sourcePaintNode.value);
            const auto residual = m * m.inverted();
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
        vector_scene::compare(ref, oracle.model(), *own.scene, *prepared.prepared->model,
                              oracle.matrices());
        ++samples;
    };
    for (std::size_t f = 0; f < 180; ++f)
        compare(f, 512, 512);
    for (std::size_t f = 180; f-- > 0;)
        compare(f, 512, 512);
    for (auto f : {179U, 0U, 90U, 15U, 14U, 15U, 179U, 0U})
        for (auto width : {128U, 512U, 128U})
            compare(f, width, width);
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
    rejects(
        [&] {
            vector_scene::compare(ref, oracle.model(), *own.scene, *prepared.prepared->model,
                                  oracle.matrices());
        },
        label);
}
int main(int argc, char **argv) {
    try {
        if (argc == 4 && std::string(argv[3]) == "--diagnostic") {
            vector_scene::collectNumericFailures = true;
            --argc;
        }
        arithmeticWitnesses();
        const auto input = vectorInput(argc, argv);
        // Direct real-Duck rectangle admission is conservatively bounded;
        // rectangular rejection probes live in the stream lifecycle test.
        run(input);
        if (argc == 1) {
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
