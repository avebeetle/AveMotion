#include "OwnPrimitiveTestData.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnNativeEllipsePreparedAsset.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "OwnJsonReader.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <iostream>
#include <stdexcept>
#include <set>
#include <array>

namespace {
using namespace avemotion;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
auto authored(const std::string& json = test::primitiveFixture()) {
    const auto document=formats::detail::readOwnJson(json);
    require(bool(document),"fixture parses");
    const auto built=runtime::detail::buildOwnNativeEllipseModel(*document.document);
    require(bool(built),"primitive authored owner ready");
    return built.prepared;
}
runtime::EvaluatedScene frame(render::detail::OwnNativeEllipseStream& stream, std::size_t index,
                              std::size_t width=280, std::size_t height=230) {
    auto emitted=stream.emit(index,width,height);
    require(bool(emitted),"seven-item stream emits");
    return std::move(*emitted.scene);
}
void streams() {
    auto prepared=render::detail::prepareOwnNativeEllipseAsset(authored());
    require(bool(prepared),"stream preparation");
    for (const auto count : {1,4,16}) {
        std::set<std::uint64_t> identities;
        std::vector<std::unique_ptr<render::detail::OwnNativeEllipseStream>> streams;
        for (int i=0;i<count;++i) {
            auto created=render::detail::OwnNativeEllipseStream::create(prepared.prepared);
            require(bool(created),"whole fixture creates seven-item stream");
            require(identities.insert(frame(*created.stream,0).instanceId).second,"independent stream identities");
            streams.push_back(std::move(created.stream));
        }
        for (auto& stream : streams) {
            std::array<std::uint64_t,61> hashes{};
            for (std::size_t f=0;f<=60;++f) {
                const auto scene=frame(*stream,f);
                require(scene.drawItems.size()==7 && scene.statistics.drawItemCount==7
                    && scene.statistics.solidPaintCount==7 && scene.layers[1].drawItemCount==7,
                    "seven ordered items and aggregate statistics");
                std::size_t points=0,verbs=0;
                for (std::size_t s=0;s<7;++s) {
                    const auto& item=scene.drawItems[s];
                    require(item.sourcePathNode==model::makeId<model::SourceNodeId>(21-3*s)
                        && item.sourcePaintNode==model::makeId<model::SourceNodeId>(22-3*s)
                        && item.modelNode==model::makeId<model::NodeId>(s) && item.drawOrder==s,
                        "reverse authored render order with unique model slots");
                    points+=item.path.points.size(); verbs+=item.path.verbs.size();
                }
                require(scene.statistics.pathPointCount==points && scene.statistics.pathVerbCount==verbs,
                    "path totals aggregate every item");
                if (f==0 || f==60) {
                    // Static ellipse at (45,115), size (58,38), sets left=45-29=16.
                    require(scene.controlBounds.valid && scene.controlBounds.left==16
                    && scene.controlBounds.top==21 && scene.controlBounds.right==(f==0?247:263)
                    && scene.controlBounds.bottom==(f==0?199:215),"literal complete fixture control bounds");
                }
                hashes[f]=scene.fingerprints.scene;
                require(runtime::computeSceneFingerprints(scene).scene==hashes[f],"full fingerprint");
            }
            for (int f=60;f>=0;--f) require(frame(*stream,static_cast<std::size_t>(f)).fingerprints.scene
                ==hashes[static_cast<std::size_t>(f)],"reverse seeks reproduce exact scene");
            const auto repeat=frame(*stream,0);
            require(!repeat.changes.firstEvaluation && !repeat.changes.visualChanged,"repeat retains visual identity");
            require(frame(*stream,0,560,460).fingerprints.scene!=hashes[0],"viewport changes transformed paths");
            require(frame(*stream,0).fingerprints.scene==hashes[0],"viewport return restores geometry");
        }
    }
    auto created=render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    auto scene=frame(*created.stream,17);
    render::MotionRenderPlanner planner;
    auto plan=planner.build(scene);
    require(bool(plan),"seven-item plan builds");
    created.stream.reset(); prepared.prepared.reset();
    require(scene.assetModel && scene.drawItems[6].canonicalGeometry
        && plan.plan.sourceScene && plan.plan.drawItems.size()==7,"scene and plan retain all resources after handles retire");
    auto intervalJson=test::replacePrimitiveOnce(test::primitiveFixture(),
        "\"ip\": 0,\n      \"op\": 61", "\"ip\": 10,\n      \"op\": 50");
    auto intervalPrepared=render::detail::prepareOwnNativeEllipseAsset(authored(intervalJson));
    auto interval=render::detail::OwnNativeEllipseStream::create(intervalPrepared.prepared);
    require(bool(interval),"active interval stream");
    require(frame(*interval.stream,0).drawItems.empty() && frame(*interval.stream,10).drawItems.size()==7
        && frame(*interval.stream,50).drawItems.empty() && frame(*interval.stream,30).drawItems.size()==7,
        "inactive/reactivated frames preserve whole group list");
}
void resources() {
    const auto input=authored();
    const auto prepared=render::detail::prepareOwnNativeEllipseAsset(input);
    require(bool(prepared),"whole fixture resources prepare");
    const auto& m=*prepared.prepared->model;
    require(m.geometries.size()==7 && m.paints.size()==7 && m.nodes.size()==7
        && m.drawOrder.size()==7 && m.layers.size()==2, "seven unique geometry/paint/draw slots");
    require(m.statistics.assetStaticGeometryCount==5 && m.statistics.assetStaticPaintCount==7,
        "five static geometries and seven static paints");
    for (std::size_t slot=0;slot<7;++slot) {
        require(m.nodes[slot].geometry==model::makeId<model::GeometryId>(slot)
            && m.nodes[slot].paint==model::makeId<model::PaintId>(slot)
            && m.nodes[slot].drawOrder==slot && m.drawOrder[slot]==m.nodes[slot].id,
            "resource slots never alias equal content");
        require(m.geometries[slot].resourceClass==(slot<2?model::ResourceClass::InstanceEvaluated:model::ResourceClass::AssetStatic)
            && bool(m.geometries[slot].staticValue)==(slot>=2),"reverse order staticness");
        require(m.paints[slot].staticValue.has_value(),"every paint has canonical content");
    }
    require(m.geometries[4].staticValue->sourceKey==5
        && m.paints[4].staticValue->sourceKey==5,
        "later static geometry and paint keep distinct canonical source identities");
    auto corrupt=std::const_pointer_cast<model::MotionAssetModel>(input->model);
    const auto saved=corrupt->properties.back().owner;
    corrupt->properties.back().owner=model::makeId<model::SourceNodeId>(19);
    const auto rejected=render::detail::prepareOwnNativeEllipseAsset(input);
    corrupt->properties.back().owner=saved;
    require(!rejected && !rejected.prepared,"later owner corruption cannot publish resources");
    require(prepared.prepared->model->properties.back().owner==saved
        && prepared.prepared->model->geometries.size()==7,"previous prepared copy survives failed replacement");
}
}
int main() {
    try {
        resources();
        streams();
        std::cout << "own primitive resources/stream tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
