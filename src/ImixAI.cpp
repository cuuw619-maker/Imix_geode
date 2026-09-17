#include "ImixAI.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace ImixAI {
namespace {
    bool F(const char* k, bool d=false){return Mod::get()->getSavedValue<bool>(k,d);}
    float V(const char* k,float d){return Mod::get()->getSavedValue<float>(k,d);}
    void SV(const char* k,float v){Mod::get()->setSavedValue(k,v);}
    int I(const char* k,int d){return Mod::get()->getSavedValue<int>(k,d);}
    void SI(const char* k,int v){Mod::get()->setSavedValue(k,v);}
    std::string S(const char* k,const std::string& d={}){return Mod::get()->getSavedValue<std::string>(k,d);}
    void SS(const char* k,const std::string& v){Mod::get()->setSavedValue(k,v);}

    struct State {
        PlayLayer* layer=nullptr;
        CCLayer* overlay=nullptr;
        CCLabelTTF* status=nullptr;
        CCLabelTTF* metrics=nullptr;
        CCLabelTTF* plan=nullptr;
        CCLabelTTF* memory=nullptr;
        CCLayerColor* scan=nullptr;
        float nextDecisionX=0.f,jumpSpacing=68.f,jumpWindow=18.f;
        float checkpointX=0.f,checkpointY=0.f,lastX=0.f,lastY=0.f;
        float time=0.f,actionTimer=0.f,deathCooldown=0.f,telemetryTimer=0.f,stuckTimer=0.f;
        float lastActionX=-100000.f,failureX=-1.f,lastFailureY=0.f;
        int failures=0,attempts=0,planRevision=0,candidate=2;
        int segmentFailures=0,segmentSuccesses=0,errorBudget=4,memoryCount=0,triedMask=0;
        bool holding=false,initialized=false,searching=false,recovering=false,hadProgress=false;
        const char* phase="BOOT";
    } s;

    int budget(){return std::min(6,4+std::min(2,std::max(0,s.segmentFailures/2)));}
    const char* candidateName(){
        switch(s.candidate){case 0:return "EARLY-2";case 1:return "EARLY";case 2:return "CENTER";case 3:return "LATE";default:return "LATE+2";}
    }
    float candidateOffset(){static const float o[5]={-4.f,-2.f,0.f,2.f,4.f};return o[std::clamp(s.candidate,0,4)];}

    CCLayer* rounded(float w,float h,float r,ccColor4F fill){
        auto root=CCLayer::create();root->setContentSize({w,h});
        auto d=CCDrawNode::create();d->setContentSize({w,h});
        std::vector<CCPoint> pts;const int n=8;const float pi=3.14159265359f;
        const float rr=std::min(r,std::min(w,h)*.5f);const float cx[4]={rr,w-rr,w-rr,rr};const float cy[4]={rr,rr,h-rr,h-rr};const float st[4]={pi,pi*1.5f,0.f,pi*.5f};
        for(int c=0;c<4;c++)for(int i=0;i<=n;i++){float a=st[c]+pi*.5f*(float(i)/n);pts.push_back({cx[c]+std::cos(a)*rr,cy[c]+std::sin(a)*rr});}
        d->drawPolygon(pts.data(),static_cast<unsigned>(pts.size()),fill,0.f,fill);root->addChild(d);return root;
    }

    void remember(const char* reason,float x,float y){
        char e[190];std::snprintf(e,sizeof(e),"%s@%.1f,%.1f[c%d,w%.2f,g%.2f]",reason,x,y,s.candidate,s.jumpWindow,s.jumpSpacing);
        auto h=S("ai-memory","");if(!h.empty())h+="|";h+=e;if(h.size()>1900)h.erase(0,h.size()-1900);SS("ai-memory",h);++s.memoryCount;
    }
    void searchLog(const char* phase,float x,float y){
        s.phase=phase;log::info("[ImixAI][SEARCH] {} x={:.1f} y={:.1f} plan={} candidate={} failures={}/{} tried=0x{:02X} checkpoint={:.1f}",phase,x,y,s.planRevision,candidateName(),s.segmentFailures,s.errorBudget,s.triedMask,s.checkpointX);
    }
    void chooseCandidate(){
        // Five deterministic hypotheses. Never reuse a hypothesis in the same segment.
        int next=-1;for(int i=0;i<5;i++){if(!(s.triedMask&(1<<i))){next=i;break;}}
        if(next<0){s.triedMask=0;for(int i=0;i<5;i++){if(i!=s.candidate){next=i;break;}}}
        s.candidate=next; s.triedMask|=(1<<s.candidate);++s.planRevision;
        s.jumpWindow=std::clamp(18.f+candidateOffset()+std::min(8,s.segmentFailures),8.f,32.f);
        s.jumpSpacing=std::clamp(68.f+candidateOffset()*2.f-s.segmentFailures*.6f,42.f,92.f);
    }
    void tap(PlayLayer* layer){
        if(!layer||s.holding||s.recovering)return;
        layer->handleButton(true,1,true);s.holding=true;s.actionTimer=0.f;
        s.lastActionX=layer->m_player1?layer->m_player1->getPositionX():s.lastActionX;
        log::info("[ImixAI][ACTION] candidate={} x={:.1f} target={:.1f}",candidateName(),s.lastActionX,s.nextDecisionX);
    }
    void release(PlayLayer* layer){if(layer&&s.holding){layer->handleButton(false,1,true);s.holding=false;}}

    void render(){
        if(!s.overlay||!s.status||!s.metrics||!s.plan||!s.memory||!s.layer||!s.layer->m_player1)return;
        auto p=s.layer->m_player1;const float x=p->getPositionX(),y=p->getPositionY(),vy=(float)p->m_yVelocity;
        char a[180],b[210],c[150],d[80];
        std::snprintf(a,sizeof(a),"IMIX AI   %s\n%s  /  PLAN %d",s.phase,s.searching?"LEARNING":"SCANNING",s.planRevision);
        std::snprintf(b,sizeof(b),"X %.1f  Y %.1f  VY %.1f\nTARGET %.1f  GAP %.1f  W %.2f",x,y,vy,s.nextDecisionX,s.jumpSpacing,s.jumpWindow);
        std::snprintf(c,sizeof(c),"CHECKPOINT %.1f\nRETRIES %d/%d  MEMORY %d",s.checkpointX,s.segmentFailures,s.errorBudget,s.memoryCount);
        std::snprintf(d,sizeof(d),"%s   %.0f%%",candidateName(),std::min(99.f,45.f+s.segmentSuccesses*8.f-s.segmentFailures*5.f));
        s.status->setString(a);s.metrics->setString(b);s.plan->setString(d);s.memory->setString(c);
        if(s.scan){s.scan->setPosition({10.f,7.f});s.scan->setScaleX(.15f+.17f*std::min(5,s.candidate+1));}
    }
}

bool enabled(){return F("ai-enabled",false);}

CCLayer* createOverlay(){
    auto root=rounded(225.f,112.f,12.f,{0.025f,0.040f,0.065f,0.96f});
    auto title=CCLabelTTF::create("IMIX AI // LOCAL AGENT","sans-serif",9.f);title->setColor({150,220,255});title->setAnchorPoint({0,1});title->setPosition({11,101});root->addChild(title);
    auto line=CCDrawNode::create();line->drawSegment({10,92},{215,92},1.f,{0.18f,0.40f,0.58f,0.9f});root->addChild(line);
    s.status=CCLabelTTF::create("BOOT","sans-serif",7.5f);s.status->setColor({235,240,248});s.status->setAnchorPoint({0,1});s.status->setPosition({11,86});root->addChild(s.status);
    s.metrics=CCLabelTTF::create("Scanning level...","sans-serif",7.f);s.metrics->setColor({190,202,218});s.metrics->setAnchorPoint({0,1});s.metrics->setPosition({11,58});root->addChild(s.metrics);
    s.plan=CCLabelTTF::create("CENTER   45%","sans-serif",7.f);s.plan->setColor({120,205,255});s.plan->setAnchorPoint({0,1});s.plan->setPosition({11,32});root->addChild(s.plan);
    s.memory=CCLabelTTF::create("CHECKPOINT 0.0\nRETRIES 0/4  MEMORY 0","sans-serif",6.8f);s.memory->setColor({135,154,174});s.memory->setAnchorPoint({0,1});s.memory->setPosition({112,32});root->addChild(s.memory);
    s.scan=CCLayerColor::create({92,184,255,150},70.f,2.f);root->addChild(s.scan,5);
    s.overlay=root;return root;
}

void reset(){
    if(s.layer)release(s.layer);s={};
    SI("ai-failures",0);SI("ai-attempts",0);SI("ai-segment-failures",0);SI("ai-segment-successes",0);SI("ai-tried-mask",0);SV("ai-last-failure-x",-1.f);SS("ai-memory","");
    log::info("[ImixAI][RESET] planner, hypotheses and persistent error memory cleared");
}

void onDeath(PlayLayer* layer){
    if(!enabled()||!layer||!layer->m_player1)return;
    auto p=layer->m_player1;
    if(s.deathCooldown>0.f){return;}
    const float x=p->getPositionX(),y=p->getPositionY();const float previous=V("ai-last-failure-x",-10000.f);const bool same=std::fabs(previous-x)<24.f;
    ++s.failures;++s.attempts;++s.segmentFailures;s.errorBudget=budget();s.failureX=x;s.lastFailureY=y;
    chooseCandidate();remember("FAIL",x,y);
    SI("ai-failures",s.failures);SI("ai-attempts",s.attempts);SI("ai-segment-failures",s.segmentFailures);SV("ai-last-failure-x",x);SV("ai-last-failure-y",y);SV("ai-spacing",s.jumpSpacing);SV("ai-window",s.jumpWindow);SI("ai-tried-mask",s.triedMask);
    // Critical recovery fix: move away from the collision immediately. Waiting for another
    // update allowed destroyPlayer to fire repeatedly at X~1.3 in the supplied log.
    const float rx=s.checkpointX,ry=s.checkpointY;p->setPosition({rx,ry});p->m_yVelocity=0.f;
    s.deathCooldown=.38f;s.recovering=true;s.searching=true;s.nextDecisionX=rx+std::max(18.f,s.jumpSpacing*.45f);s.lastActionX=-100000.f;
    log::warn("[ImixAI][FAIL] x={:.1f} y={:.1f} same-region={} -> {} plan={} retries={}/{}",x,y,same,candidateName(),s.planRevision,s.segmentFailures,s.errorBudget);
    log::info("[ImixAI][ROLLBACK] immediate safe rollback x={:.1f} y={:.1f}; collision debounce=380ms",rx,ry);
}

void update(PlayLayer* layer,float dt){
    if(!enabled()||!layer||!layer->m_player1)return;
    auto p=layer->m_player1;
    if(s.layer!=layer){
        if(s.layer)release(s.layer);s.layer=layer;s.initialized=false;s.recovering=false;s.failures=I("ai-failures",0);s.attempts=I("ai-attempts",0);s.segmentFailures=I("ai-segment-failures",0);s.segmentSuccesses=I("ai-segment-successes",0);s.triedMask=I("ai-tried-mask",0);s.jumpSpacing=std::clamp(V("ai-spacing",68.f),42.f,92.f);s.jumpWindow=std::clamp(V("ai-window",18.f),8.f,32.f);s.errorBudget=budget();log::info("[ImixAI][BOOT] autonomous local planner online; adaptive budget={}",s.errorBudget);
    }
    s.time+=dt;s.actionTimer+=dt;s.telemetryTimer+=dt;s.deathCooldown=std::max(0.f,s.deathCooldown-dt);
    const float x=p->getPositionX(),y=p->getPositionY(),vy=(float)p->m_yVelocity;
    if(!s.initialized){s.lastX=x;s.lastY=y;s.checkpointX=x;s.checkpointY=y;s.nextDecisionX=x+18.f;s.initialized=true;s.phase="SCANNING LEVEL";s.errorBudget=budget();searchLog("SCANNING LEVEL",x,y);}
    const float dx=x-s.lastX,dy=y-s.lastY;s.lastX=x;s.lastY=y;
    if(dx>0.15f){s.hadProgress=true;s.stuckTimer=0.f;}else s.stuckTimer+=dt;
    if(s.recovering&&s.deathCooldown<=0.f){s.recovering=false;s.phase="SEARCHING ALTERNATIVE";searchLog(s.phase,x,y);}
    if(s.holding&&s.actionTimer>.052f)release(layer);

    // A checkpoint is committed only after meaningful forward progress. This prevents
    // the start checkpoint from becoming a bad recovery point.
    if(s.hadProgress&&x>s.checkpointX+105.f){s.checkpointX=x-42.f;s.checkpointY=y;s.segmentSuccesses++;s.segmentFailures=0;s.triedMask=0;s.errorBudget=budget();SI("ai-segment-successes",s.segmentSuccesses);SI("ai-segment-failures",0);SI("ai-tried-mask",0);log::info("[ImixAI][CHECKPOINT] safe progress checkpoint x={:.1f} y={:.1f}",s.checkpointX,s.checkpointY);}

    // If the game is moving, the planner predicts the next interaction. If it is not
    // moving, send one recovery input rather than spamming inputs every frame.
    if(s.stuckTimer>.40f&&!s.holding&&!s.recovering){
        s.phase="RECOVERING INPUT";tap(layer);s.stuckTimer=0.f;
    }
    const float trigger=std::max(8.f,s.jumpWindow);
    if(!s.recovering&&!s.holding&&x+trigger>=s.nextDecisionX&&x-s.lastActionX>14.f){
        s.searching=true;s.phase="TESTING CANDIDATE";tap(layer);s.nextDecisionX+=s.jumpSpacing;searchLog(s.phase,x,y);
    }else if(s.searching){
        s.phase=vy>1.f?"VERIFY ASCENT":vy<-1.f?"VERIFY DESCENT":"ANALYZING TRAJECTORY";
    }else s.phase="OBSERVING";

    if(s.telemetryTimer>.25f){s.telemetryTimer=0.f;log::debug("[ImixAI][TELEMETRY] x={:.1f} y={:.1f} vy={:.1f} dx={:.2f} next={:.1f} candidate={} plan={} stuck={:.2f}",x,y,vy,dx,s.nextDecisionX,candidateName(),s.planRevision,s.stuckTimer);}
    render();SV("ai-spacing",s.jumpSpacing);SV("ai-window",s.jumpWindow);
}
}
