#include "NativeEllipseCertificate.hpp"
#include "NativeEllipseOracle.hpp"
#include "OwnPrimitiveTestData.hpp"
#include "OwnPrimitiveSceneComparison.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "NativeEllipseEvaluationHelpers.hpp"
#include "avemotion/render/RenderPlanner.hpp"

#include <iostream>
#include <functional>
#include <tuple>

namespace {
using namespace avemotion;
using namespace model;
constexpr std::uint64_t referenceId=0x26A26004ULL;
void require(bool ok,const std::string& why) { if(!ok) throw std::runtime_error(why); }

auto sourceFields(const MotionSourceNodeRecord& n) {
    return std::tuple{n.present,n.kind,n.layerKind,n.nameHash,n.debugName,n.hidden,n.authoredStatic,
        n.autoOrient,n.authoredLayerId,n.authoredParentLayerId,n.inFrame,n.outFrame,n.startFrame,n.timeStretch,
        n.dependencyBits,n.fillRule,n.strokeCap,n.strokeJoin,n.gradientType,n.maskMode,n.matteMode,
        n.blendMode,n.pathDirection,n.polystarType,n.trimMode,n.miterLimit,n.repeaterMaximumCopies,
        n.gradientColorPointCount,n.layerWidth,n.layerHeight,n.solidColor,n.sourceAssetRefHash,n.enabled,n.maskInverted};
}
std::vector<PropertyId> propertyRoles(const runtime::detail::OwnPrimitiveBinding& b) {
    std::vector<PropertyId> result{b.layerTransform,b.layerOpacity};
    for(const auto& g:b.groups) {
        for(auto id:{g.groupTransform,g.groupOpacity,g.position,g.size}) result.push_back(id);
        if(g.roundness) result.push_back(*g.roundness);
        result.push_back(g.color); result.push_back(g.fillOpacity);
    }
    return result;
}
std::vector<SourceNodeId> nodeRoles(const runtime::detail::OwnPrimitiveBinding& b) {
    std::vector<SourceNodeId> result{b.root,b.layer};
    for(const auto& g:b.groups) for(auto id:{g.group,g.primitive,g.fill}) result.push_back(id);
    return result;
}
void models(const MotionAssetModel& reference,const runtime::detail::OwnPrimitiveBinding& rb,
            const MotionAssetModel& own,const runtime::detail::OwnPrimitiveBinding& ob) {
    const auto rn=nodeRoles(rb),on=nodeRoles(ob);
    for(std::size_t i=0;i<rn.size();++i) {
        const auto& a=reference.sourceNodes[rn[i].index()]; const auto& b=own.sourceNodes[on[i].index()];
        require(sourceFields(a)==sourceFields(b),"all source semantics role="+std::to_string(i));
        require(!a.transformParent.valid() && !b.transformParent.valid()
            && !a.referencedComposition.valid() && !b.referencedComposition.valid(),"unsupported node references absent");
    }
    const auto rp=propertyRoles(rb),op=propertyRoles(ob);
    const auto value=[&](MotionValueRef a,MotionValueRef b) {
        require(a.valid() && b.valid() && a.type==b.type,"value references have same type");
        bool same=false;
        switch(a.type) {
        case PropertyValueType::Scalar:same=reference.scalarValues.at(a.index)==own.scalarValues.at(b.index);break;
        case PropertyValueType::Vec2:same=reference.vec2Values.at(a.index)==own.vec2Values.at(b.index);break;
        case PropertyValueType::Color:same=reference.colorValues.at(a.index)==own.colorValues.at(b.index);break;
        case PropertyValueType::Matrix3x2:same=reference.matrixValues.at(a.index)==own.matrixValues.at(b.index);break;
        default:break;
        }
        require(same,"full typed value equality");
    };
    for(std::size_t i=0;i<rp.size();++i) {
        const auto& a=reference.properties[rp[i].index()];const auto& b=own.properties[op[i].index()];
        require(a.semantic==b.semantic && a.valueType==b.valueType && a.flags==b.flags
            && a.semanticIndex==b.semanticIndex,"property semantic/type/flags");
        if(a.flags==PropertyFlagStatic) { require(!a.track.valid() && !b.track.valid(),"static track absent"); value(a.staticValue,b.staticValue); }
        else {
            require(!a.staticValue.valid() && !b.staticValue.valid(),"animated static ref absent");
            const auto& at=reference.tracks.at(a.track.index());const auto& bt=own.tracks.at(b.track.index());
            require(at.present && bt.present && at.id==a.track && bt.id==b.track && at.property==a.id
                && bt.property==b.id && at.firstFrame==bt.firstFrame && at.endFrame==bt.endFrame
                && at.segments.count==1 && bt.segments.count==1,"track role/timing/range");
            const auto& as=reference.segments.at(at.segments.first);const auto& bs=own.segments.at(bt.segments.first);
            require(as.present && bs.present && as.track==at.id && bs.track==bt.id
                && as.firstFrame==bs.firstFrame && as.endFrame==bs.endFrame
                && as.interpolation==bs.interpolation && as.spatialInterpolation==bs.spatialInterpolation
                && as.temporalControl1==bs.temporalControl1 && as.temporalControl2==bs.temporalControl2
                && as.spatialInTangent==bs.spatialInTangent && as.spatialOutTangent==bs.spatialOutTangent,
                "all segment semantics");
            value(as.startValue,bs.startValue);value(as.endValue,bs.endValue);
        }
    }
}
void run(const std::string& json,const std::string& label) {
    auto owner=test::prepareOwnEllipseForTest(json);
    auto stream=render::detail::OwnNativeEllipseStream::create(owner);
    require(bool(stream),"own stream creates");
    test::NativeEllipseOracle oracle(json);
    const auto rb=test::primitiveTestRoles(*oracle.model()),ob=test::primitiveTestRoles(*owner->model);
    models(*oracle.model(),rb,*owner->model,ob);
    render::MotionRenderPlanner referencePlanner,ownPlanner;
    const test::OwnPrimitiveSceneComparison comparator;
    runtime::SceneFingerprints prevR{},prevO{};
    std::uint64_t ordinal=0;
    const auto compare=[&](std::size_t f,std::size_t width=280,std::size_t height=230) {
        auto r=oracle.freshScene(f,width,height);auto emitted=stream.stream->emit(f,width,height);
        require(bool(emitted),"own emission");auto o=*emitted.scene;
        require(r.instanceId==0 && r.evaluationSequence==++ordinal && o.evaluationSequence==ordinal,"sequence and raw oracle");
        require(r.changes.firstEvaluation==(ordinal==1) && o.changes.firstEvaluation==(ordinal==1),"first history");
        if(ordinal>1) for(const auto& pair:{std::pair{&r,&prevR},std::pair{&o,&prevO}}) {
            const auto& s=*pair.first;const auto& p=*pair.second;
            require(s.changes.topologyChanged==(s.fingerprints.topology!=p.topology)
                && s.changes.geometryChanged==(s.fingerprints.geometry!=p.geometry)
                && s.changes.paintChanged==(s.fingerprints.paint!=p.paint)
                && s.changes.visualChanged==(s.fingerprints.scene!=p.scene),"complete history");
        }
        prevR=r.fingerprints;prevO=o.fingerprints;r.instanceId=referenceId;
        const auto context=label+" frame="+std::to_string(f)+" ordinal="+std::to_string(ordinal);
        require(comparator.sceneDifference(r,rb,o,ob,referenceId).empty(),context+" "+comparator.sceneDifference(r,rb,o,ob,referenceId));
        auto rp=referencePlanner.build(r),op=ownPlanner.build(o);
        require(bool(rp)&&bool(op),"both plans build");
        const auto diff=comparator.planDifference(rp.plan,rb,op.plan,ob,referenceId);
        require(diff.empty(),context+" "+diff);
        if(ordinal==1) {
            const std::vector<std::function<void(runtime::EvaluatedScene&)>> corruptions{
                [](auto& s){s.drawItems[6].path.points[0].x+=1;},
                [](auto& s){s.drawItems[5].paint.solid.r^=1;},
                [](auto& s){std::swap(s.drawItems[0],s.drawItems[1]);},
                [](auto& s){s.drawItems[4].sourcePathNode=s.drawItems[3].sourcePathNode;},
                [](auto& s){s.drawItems[3].localPath.points.back().y+=1;}
            };
            for(const auto& mutate:corruptions) { auto changed=o;mutate(changed);
                changed.fingerprints=runtime::computeSceneFingerprints(changed);
                require(!comparator.sceneDifference(r,rb,changed,ob,referenceId).empty(),"multi-item comparator mutation witness"); }
            auto changed=op.plan;std::swap(changed.drawItems[0],changed.drawItems[1]);
            changed.fingerprints=render::computeRenderPlanFingerprints(changed);
            require(!comparator.planDifference(rp.plan,rb,changed,ob,referenceId).empty(),"plan order witness");
        }
    };
    for(std::size_t f=0;f<=60;++f) compare(f);
    for(int f=60;f>=0;--f) compare(static_cast<std::size_t>(f));
    compare(17);compare(17);compare(17,400,300);compare(0);
    require(oracle.directParseCount()==2 && oracle.directSampleCount()==61+ordinal,"fresh ordinary renderTree per sample");
    std::cout<<label<<" comparisons="<<ordinal<<"\n";
}
}
int main() {
    try {
        const auto json=test::primitiveFixture();run(json,"whole");
        // Overlap green rounded rectangle with red sharp rectangle: reverse draw
        // order makes red visibly cover green; separated fixture pixels cannot prove this.
        run(test::replacePrimitiveOnce(json,"125,\n                  42","45,\n                  42"),"overlap");
        std::cout<<"own primitive differential passed\n";return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
