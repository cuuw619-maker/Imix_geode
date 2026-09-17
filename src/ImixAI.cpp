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
    bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
    float V(const char* k, float d) { return Mod::get()->getSavedValue<float>(k, d); }
    void SV(const char* k, float v) { Mod::get()->setSavedValue(k, v); }
    int I(const char* k, int d) { return Mod::get()->getSavedValue<int>(k, d); }
    void SI(const char* k, int v) { Mod::get()->setSavedValue(k, v); }
    std::string S(const char* k, const std::string& d = {}) { return Mod::get()->getSavedValue<std::string>(k, d); }
    void SS(const char* k, const std::string& v) { Mod::get()->setSavedValue(k, v); }

    struct Failure { float x=0, y=0, window=0, spacing=0; int candidate=1; };
    struct State {
        PlayLayer* layer = nullptr;
        CCLayer* overlay = nullptr;
        CCLabelTTF* status = nullptr;
        CCLabelTTF* metrics = nullptr;
        CCLabelTTF* memory = nullptr;
        float nextDecisionX = 70.f, jumpSpacing = 86.f, jumpWindow = 24.f;
        float checkpointX = 0.f, checkpointY = 0.f, lastX = 0.f, lastY = 0.f;
        float time = 0.f, actionTimer = 0.f, deathCooldown = 0.f, telemetryTimer = 0.f;
        float lastActionX = -100000.f, failureX = -1.f, lastFailureY = 0.f;
        int failures = 0, attempts = 0, planRevision = 0, candidate = 1;
        int segmentFailures = 0, segmentSuccesses = 0, errorBudget = 4, memoryCount = 0;
        bool holding = false, initialized = false, searching = false, rollbackPending = false;
        const char* phase = "BOOT";
    } s;

    int budget() { return std::min(6, 4 + std::min(2, std::max(0, s.segmentFailures / 2))); }
    const char* candidateName() { return s.candidate == 0 ? "EARLY" : s.candidate == 2 ? "LATE" : "CENTER"; }

    void remember(const char* reason, float x, float y) {
        char entry[180];
        std::snprintf(entry, sizeof(entry), "%s@%.1f,%.1f[%s,w%.1f,g%.1f]", reason, x, y, candidateName(), s.jumpWindow, s.jumpSpacing);
        auto history = S("ai-memory", "");
        if (!history.empty()) history += "|";
        history += entry;
        if (history.size() > 1900) history.erase(0, history.size() - 1900);
        SS("ai-memory", history);
        ++s.memoryCount;
    }

    void logState(const char* phase, float x, float y) {
        s.phase = phase;
        log::info("[ImixAI][SEARCH] phase={} x={:.1f} y={:.1f} plan={} candidate={} failures={} budget={} checkpoint={:.1f}", phase, x, y, s.planRevision, candidateName(), s.segmentFailures, s.errorBudget, s.checkpointX);
    }

    CCLayer* panel(float w, float h, float r) {
        auto out = CCLayer::create(); out->setContentSize({w,h});
        auto d = CCDrawNode::create(); d->setContentSize({w,h});
        std::vector<CCPoint> pts; const int n=8; const float pi=3.14159265359f; const float rr=std::min(r,std::min(w,h)*.5f);
        const float cx[4]={rr,w-rr,w-rr,rr}, cy[4]={rr,rr,h-rr,h-rr}, st[4]={pi,pi*1.5f,0.f,pi*.5f};
        for(int c=0;c<4;c++) for(int i=0;i<=n;i++){float a=st[c]+pi*.5f*(float(i)/n);pts.push_back({cx[c]+std::cos(a)*rr,cy[c]+std::sin(a)*rr});}
        d->drawPolygon(pts.data(), (unsigned)pts.size(), {0.035f,0.055f,0.08f,0.94f}, 1.0f, {0.16f,0.24f,0.34f,0.9f});
        out->addChild(d); return out;
    }

    void render() {
        if (!s.overlay || !s.status || !s.metrics || !s.memory) return;
        auto p = s.layer ? s.layer->m_player1 : nullptr;
        if (!p) return;
        char a[180], b[220], c[160];
        std::snprintf(a,sizeof(a),"IMIX AI   %s\n%s  •  PLAN %d  •  %s",s.phase,s.searching?"SEARCHING":"SCANNING",s.planRevision,candidateName());
        std::snprintf(b,sizeof(b),"X %.1f   Y %.1f   VY %.1f\nNEXT %.1f   GAP %.1f   WINDOW %.1f\nCHECKPOINT %.1f   RETRIES %d/%d",p->getPositionX(),p->getPositionY(),(float)p->m_yVelocity,s.nextDecisionX,s.jumpSpacing,s.jumpWindow,s.checkpointX,s.segmentFailures,s.errorBudget);
        std::snprintf(c,sizeof(c),"MEMORY %d   SUCCESS %d\nlast error %.1f / %.1f",s.memoryCount,s.segmentSuccesses,s.failureX,s.lastFailureY);
        s.status->setString(a); s.metrics->setString(b); s.memory->setString(c);
    }

    void tap(PlayLayer* layer) {
        if (!layer || s.holding) return;
        layer->handleButton(true, 1, true); s.holding=true; s.actionTimer=0.f;
        s.lastActionX=layer->m_player1 ? layer->m_player1->getPositionX() : s.lastActionX;
        log::info("[ImixAI][ACTION] jump candidate={} x={:.1f} window={:.1f}",candidateName(),s.lastActionX,s.jumpWindow);
    }
    void release(PlayLayer* layer) { if(layer && s.holding){layer->handleButton(false,1,true);s.holding=false;} }
    void chooseNextCandidate() {
        if(s.candidate==1)s.candidate=0; else if(s.candidate==0)s.candidate=2; else s.candidate=1;
        ++s.planRevision;
        const float phase=s.candidate==0?-1.f:s.candidate==2?1.f:0.f;
        s.jumpWindow=std::clamp(22.f+phase*4.f+s.segmentFailures*1.2f,8.f,38.f);
        s.jumpSpacing=std::clamp(s.jumpSpacing+phase*2.f-0.5f,44.f,120.f);
    }
}

bool enabled(){return F("ai-enabled",false);}

CCLayer* createOverlay(){
    auto root=panel(205.f,91.f,10.f);
    auto title=CCLabelTTF::create("IMIX AI","sans-serif",10.f); title->setColor({150,220,255}); title->setAnchorPoint({0,1}); title->setPosition({10,82}); root->addChild(title);
    auto line=CCDrawNode::create(); line->drawSegment({9,73},{196,73},1.f,{0.22f,0.42f,0.58f,0.8f}); root->addChild(line);
    s.overlay=root;
    s.status=CCLabelTTF::create("BOOT","sans-serif",7.5f); s.status->setColor({235,240,248}); s.status->setAnchorPoint({0,1}); s.status->setPosition({10,68}); root->addChild(s.status);
    s.metrics=CCLabelTTF::create("Scanning...","sans-serif",7.f); s.metrics->setColor({190,202,218}); s.metrics->setAnchorPoint({0,1}); s.metrics->setPosition({10,42}); root->addChild(s.metrics);
    s.memory=CCLabelTTF::create("MEMORY 0","sans-serif",6.8f); s.memory->setColor({135,154,174}); s.memory->setAnchorPoint({0,1}); s.memory->setPosition({10,17}); root->addChild(s.memory);
    return root;
}

void reset(){
    if(s.layer)release(s.layer); s={};
    SI("ai-failures",0);SI("ai-attempts",0);SI("ai-segment-failures",0);SI("ai-segment-successes",0);SV("ai-last-failure-x",-1.f);SS("ai-memory","");
    log::info("[ImixAI][RESET] persistent error memory and planner state cleared");
}

void onDeath(PlayLayer* layer){
    if(!enabled()||!layer||!layer->m_player1)return;
    if(s.deathCooldown>0.f){log::debug("[ImixAI][FAIL] duplicate death callback ignored during recovery");return;}
    auto p=layer->m_player1; const float x=p->getPositionX(), y=p->getPositionY(); const float previous=V("ai-last-failure-x",-1.f);
    ++s.failures;++s.attempts;++s.segmentFailures;s.failureX=x;s.lastFailureY=y;s.errorBudget=budget();
    const bool sameRegion=previous>0.f&&std::fabs(previous-x)<24.f;
    chooseNextCandidate(); if(!sameRegion)s.jumpWindow=std::clamp(s.jumpWindow-1.f,8.f,40.f);
    SV("ai-last-failure-x",x);SV("ai-last-failure-y",y);SV("ai-spacing",s.jumpSpacing);SV("ai-window",s.jumpWindow);SI("ai-failures",s.failures);SI("ai-attempts",s.attempts);SI("ai-segment-failures",s.segmentFailures);SI("ai-segment-successes",s.segmentSuccesses);
    remember("FAIL",x,y);
    s.deathCooldown=.22f; s.rollbackPending=true; s.searching=true;
    log::warn("[ImixAI][FAIL] x={:.1f} y={:.1f} same-region={} -> candidate={} plan={} errors={}/{}",x,y,sameRegion,candidateName(),s.planRevision,s.segmentFailures,s.errorBudget);
    log::info("[ImixAI][ROLLBACK] scheduling recovery to x={:.1f} y={:.1f}",s.checkpointX,s.checkpointY);
}

void update(PlayLayer* layer,float dt){
    if(!enabled()||!layer||!layer->m_player1)return; auto p=layer->m_player1;
    if(s.layer!=layer){if(s.layer)release(s.layer);s.layer=layer;s.initialized=false;s.failures=I("ai-failures",0);s.attempts=I("ai-attempts",0);s.segmentFailures=I("ai-segment-failures",0);s.segmentSuccesses=I("ai-segment-successes",0);s.jumpSpacing=std::clamp(V("ai-spacing",86.f),44.f,120.f);s.jumpWindow=std::clamp(V("ai-window",24.f),8.f,40.f);s.errorBudget=budget();log::info("[ImixAI][BOOT] adaptive local gameplay agent online; budget={}",s.errorBudget);}
    s.time+=dt;s.actionTimer+=dt;s.deathCooldown=std::max(0.f,s.deathCooldown-dt);s.telemetryTimer+=dt;
    const float x=p->getPositionX(),y=p->getPositionY(),vy=(float)p->m_yVelocity;
    if(!s.initialized){s.lastX=x;s.lastY=y;s.checkpointX=x;s.checkpointY=y;s.nextDecisionX=x+45.f;s.errorBudget=budget();s.initialized=true;logState("SCANNING LEVEL",x,y);}
    const float dx=x-s.lastX,dy=y-s.lastY;s.lastX=x;s.lastY=y;

    if(s.rollbackPending && s.deathCooldown<=0.f){
        const float rx=s.checkpointX, ry=s.checkpointY; p->setPosition({rx,ry}); p->m_yVelocity=0.f; s.rollbackPending=false; s.nextDecisionX=rx+38.f; s.lastActionX=-100000.f;
        log::info("[ImixAI][ROLLBACK] applied checkpoint x={:.1f} y={:.1f}; planner resumed",rx,ry);
    }
    if(dx>0.01f&&x>s.checkpointX+120.f){s.checkpointX=x-55.f;s.checkpointY=y;++s.segmentSuccesses;s.segmentFailures=0;s.errorBudget=budget();log::info("[ImixAI][CHECKPOINT] committed x={:.1f} y={:.1f}",s.checkpointX,s.checkpointY);}
    if(s.holding&&s.actionTimer>.055f)release(layer);

    // Start by observing the player; then use a deterministic short-horizon cadence.
    // A new action is only emitted after meaningful horizontal progress.
    if(!s.rollbackPending && x+s.jumpWindow>=s.nextDecisionX && x-s.lastActionX>18.f){s.searching=true;tap(layer);s.nextDecisionX+=s.jumpSpacing;logState("TESTING CANDIDATE",x,y);}
    else if(s.searching&&std::fabs(dy)>2.f){s.phase=vy>0.f?"VERIFY ASCENT":"VERIFY DESCENT";}
    else s.phase="ANALYZING / PREDICTING";

    if(s.telemetryTimer>.20f){s.telemetryTimer=0.f;log::debug("[ImixAI][TELEMETRY] x={:.1f} y={:.1f} vy={:.1f} next={:.1f} candidate={} plan={} rollback={}",x,y,vy,s.nextDecisionX,candidateName(),s.planRevision,s.rollbackPending);}
    render(); SV("ai-spacing",s.jumpSpacing);SV("ai-window",s.jumpWindow);
}
}
