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
    bool F(const char* k,bool d=false){return Mod::get()->getSavedValue<bool>(k,d);}
    float V(const char* k,float d){return Mod::get()->getSavedValue<float>(k,d);}
    void SV(const char* k,float v){Mod::get()->setSavedValue(k,v);}
    int I(const char* k,int d){return Mod::get()->getSavedValue<int>(k,d);}
    void SI(const char* k,int v){Mod::get()->setSavedValue(k,v);}
    std::string S(const char* k,const std::string& d={}){return Mod::get()->getSavedValue<std::string>(k,d);}
    void SS(const char* k,const std::string& v){Mod::get()->setSavedValue(k,v);}

    struct Hazard{float x=0.f,y=0.f;int id=0;float score=0.f;};
    struct State{
        PlayLayer* layer=nullptr; CCLayer* overlay=nullptr;
        CCLabelTTF* status=nullptr; CCLabelTTF* metrics=nullptr; CCLabelTTF* plan=nullptr; CCLabelTTF* memory=nullptr;
        CCLayerColor* scan=nullptr;
        float time=0.f,actionTimer=0.f,scanTimer=0.f,telemetryTimer=0.f,stuckTimer=0.f,checkpointTimer=0.f;
        float lastX=0.f,lastY=0.f,checkpointX=0.f,checkpointY=0.f;
        float targetX=-1.f,targetY=0.f,targetGap=0.f,nextActionX=0.f,lastActionX=-100000.f,speed=0.f,jumpLead=28.f;
        int targetID=0,candidate=2,planRevision=0,segmentFailures=0,segmentSuccesses=0;
        int failures=0,attempts=0,errorBudget=4,triedMask=0,memoryCount=0,hazardCount=0;
        bool initialized=false,holding=false,searching=false,deathSeen=false,checkpointQueued=false,hadProgress=false;
        const char* phase="BOOT";
    } s;

    int budget(){return std::min(6,4+std::min(2,std::max(0,s.segmentFailures/2)));}
    const char* candidateName(){switch(s.candidate){case 0:return "EARLY-2";case 1:return "EARLY";case 2:return "CENTER";case 3:return "LATE";default:return "LATE+2";}}
    float candidateOffset(){static const float a[5]={-6.f,-3.f,0.f,3.f,6.f};return a[std::clamp(s.candidate,0,4)];}

    bool isHazard(GameObject* o){
        if(!o)return false;const int id=o->m_objectID;
        switch(id){case 8:case 39:case 88:case 103:case 104:case 105:case 106:case 140:case 141:case 142:case 143:case 144:case 145:return true;default:return false;}
    }

    CCLayer* rounded(float w,float h,float r,ccColor4F fill){
        auto root=CCLayer::create();root->setContentSize({w,h});auto d=CCDrawNode::create();d->setContentSize({w,h});
        std::vector<CCPoint> pts;const int n=8;const float pi=3.14159265359f;const float rr=std::min(r,std::min(w,h)*.5f);
        const float cx[4]={rr,w-rr,w-rr,rr},cy[4]={rr,rr,h-rr,h-rr},st[4]={pi,pi*1.5f,0.f,pi*.5f};
        for(int c=0;c<4;c++)for(int i=0;i<=n;i++){float a=st[c]+pi*.5f*(float(i)/n);pts.push_back({cx[c]+std::cos(a)*rr,cy[c]+std::sin(a)*rr});}
        d->drawPolygon(pts.data(),static_cast<unsigned>(pts.size()),fill,0.f,fill);root->addChild(d);return root;
    }

    void remember(const char* reason,float x,float y){
        char e[180];std::snprintf(e,sizeof(e),"%s@%.1f,%.1f[id%d,c%d,g%.1f]",reason,x,y,s.targetID,s.candidate,s.targetGap);
        auto h=S("ai-memory","");if(!h.empty())h+="|";h+=e;if(h.size()>1900)h.erase(0,h.size()-1900);SS("ai-memory",h);++s.memoryCount;
    }
    void searchLog(const char* phase,float x,float y){
        s.phase=phase;log::info("[ImixAI][SEARCH] {} x={:.1f} y={:.1f} target={:.1f} id={} speed={:.2f} candidate={} retries={}/{} hazards={}",phase,x,y,s.targetX,s.targetID,s.speed,candidateName(),s.segmentFailures,s.errorBudget,s.hazardCount);
    }
    void chooseCandidate(){
        int next=-1;for(int i=0;i<5;i++)if(!(s.triedMask&(1<<i))){next=i;break;}
        if(next<0){s.triedMask=0;for(int i=0;i<5;i++)if(i!=s.candidate){next=i;break;}}
        s.candidate=next;s.triedMask|=(1<<s.candidate);++s.planRevision;s.jumpLead=std::clamp(28.f+candidateOffset()+std::min(12,s.segmentFailures*2),12.f,48.f);
    }

    void scanLevel(float x,float y){
        if(!s.layer||!s.layer->m_objects)return;Hazard best;best.x=999999.f;s.hazardCount=0;
        for(auto object:geode::cocos::CCArrayExt<GameObject,false>(s.layer->m_objects)){
            if(!isHazard(object))continue;auto pos=object->getPosition();float dx=pos.x-x;if(dx<-8.f||dx>190.f)continue;
            ++s.hazardCount;float score=dx+std::fabs(pos.y-y)*.12f;if(score<best.score||best.x==999999.f)best={pos.x,pos.y,object->m_objectID,score};
        }
        if(best.x<999000.f){s.targetX=best.x;s.targetY=best.y;s.targetID=best.id;s.targetGap=best.x-x;s.nextActionX=best.x-s.jumpLead;}
        else{s.targetX=-1.f;s.targetID=0;s.targetGap=-1.f;s.nextActionX=x+std::clamp(72.f-s.speed*4.f,52.f,86.f);}
        ++s.planRevision;s.searching=true;log::debug("[ImixAI][SCAN] x={:.1f} hazards={} target={:.1f} id={} lead={:.1f}",x,s.hazardCount,s.targetX,s.targetID,s.jumpLead);
    }

    void tap(PlayLayer* layer){
        if(!layer||s.holding)return;layer->handleButton(true,1,true);s.holding=true;s.actionTimer=0.f;
        s.lastActionX=layer->m_player1?layer->m_player1->getPositionX():s.lastActionX;s.phase="INPUT";
        log::info("[ImixAI][ACTION] JUMP candidate={} x={:.1f} target={:.1f} gap={:.1f} id={}",candidateName(),s.lastActionX,s.targetX,s.targetGap,s.targetID);
    }
    void release(PlayLayer* layer){if(layer&&s.holding){layer->handleButton(false,1,true);s.holding=false;}}

    void queueCheckpoint(float x,float y){
        if(!s.layer||!s.layer->m_isPracticeMode||s.checkpointQueued)return;
        s.layer->queueCheckpoint();s.checkpointQueued=true;s.checkpointTimer=.25f;s.checkpointX=x;s.checkpointY=y;
        log::info("[ImixAI][CHECKPOINT] queued built-in practice checkpoint x={:.1f} y={:.1f}",x,y);
    }

    void render(){
        if(!s.overlay||!s.status||!s.metrics||!s.plan||!s.memory||!s.layer||!s.layer->m_player1)return;
        auto p=s.layer->m_player1;float x=p->getPositionX(),y=p->getPositionY(),vy=(float)p->m_yVelocity;char a[180],b[220],c[150],d[120];
        std::snprintf(a,sizeof(a),"IMIX AI   %s\nPRACTICE  •  %s",s.phase,s.searching?"ANALYZING":"OBSERVING");
        std::snprintf(b,sizeof(b),"X %.1f  Y %.1f  VY %.1f\nHAZARD %.1f  GAP %.1f  LEAD %.1f",x,y,vy,s.targetX,s.targetGap,s.jumpLead);
        std::snprintf(c,sizeof(c),"CP %.1f   HAZARDS %d\nRETRY %d/%d   MEMORY %d",s.checkpointX,s.hazardCount,s.segmentFailures,s.errorBudget,s.memoryCount);
        std::snprintf(d,sizeof(d),"%s  %d%%  ID:%d",candidateName(),std::clamp(45+s.segmentSuccesses*7-s.segmentFailures*5,5,99),s.targetID);
        s.status->setString(a);s.metrics->setString(b);s.plan->setString(d);s.memory->setString(c);if(s.scan){s.scan->setPosition({10.f,7.f});s.scan->setScaleX(.2f+.12f*std::min(6,s.hazardCount));}
    }
}

bool enabled(){return F("ai-enabled",false);}

CCLayer* createOverlay(){
    auto root=rounded(245.f,120.f,12.f,{0.018f,0.032f,0.055f,.97f});
    auto title=CCLabelTTF::create("IMIX AI // LOCAL AUTOPILOT","sans-serif",9.f);title->setColor({150,220,255});title->setAnchorPoint({0,1});title->setPosition({11,109});root->addChild(title);
    auto line=CCDrawNode::create();line->drawSegment({10,99},{235,99},1.f,{0.18f,0.40f,0.58f,.9f});root->addChild(line);
    s.status=CCLabelTTF::create("BOOT","sans-serif",7.5f);s.status->setColor({235,240,248});s.status->setAnchorPoint({0,1});s.status->setPosition({11,92});root->addChild(s.status);
    s.metrics=CCLabelTTF::create("Scanning level...","sans-serif",7.f);s.metrics->setColor({190,202,218});s.metrics->setAnchorPoint({0,1});s.metrics->setPosition({11,64});root->addChild(s.metrics);
    s.plan=CCLabelTTF::create("CENTER 45%","sans-serif",7.f);s.plan->setColor({120,205,255});s.plan->setAnchorPoint({0,1});s.plan->setPosition({11,37});root->addChild(s.plan);
    s.memory=CCLabelTTF::create("CP 0.0\nRETRY 0/4 MEMORY 0","sans-serif",6.8f);s.memory->setColor({135,154,174});s.memory->setAnchorPoint({0,1});s.memory->setPosition({128,37});root->addChild(s.memory);
    s.scan=CCLayerColor::create({92,184,255,150},70.f,2.f);root->addChild(s.scan,5);s.overlay=root;return root;
}

void reset(){
    if(s.layer)release(s.layer);s={};SI("ai-failures",0);SI("ai-attempts",0);SI("ai-segment-failures",0);SI("ai-segment-successes",0);SI("ai-tried-mask",0);SV("ai-last-failure-x",-1.f);SS("ai-memory","");
    log::info("[ImixAI][RESET] planner and persistent error memory cleared");
}

void onDeath(PlayLayer* layer){
    if(!enabled()||!layer||!layer->m_player1||s.deathSeen)return;
    s.deathSeen=true;++s.failures;++s.attempts;++s.segmentFailures;s.errorBudget=budget();float x=layer->m_player1->getPositionX(),y=layer->m_player1->getPositionY();
    chooseCandidate();remember("FAIL",x,y);SI("ai-failures",s.failures);SI("ai-attempts",s.attempts);SI("ai-segment-failures",s.segmentFailures);SI("ai-tried-mask",s.triedMask);SV("ai-last-failure-x",x);SV("ai-last-failure-y",y);s.phase="DEATH / PRACTICE RESPAWN";release(layer);
    log::warn("[ImixAI][FAIL] x={:.1f} y={:.1f} -> candidate={} retry={}/{}",x,y,candidateName(),s.segmentFailures,s.errorBudget);
}

void update(PlayLayer* layer,float dt){
    if(!enabled()||!layer||!layer->m_player1)return;auto p=layer->m_player1;
    if(s.layer!=layer){if(s.layer)release(s.layer);s.layer=layer;s.initialized=false;s.deathSeen=false;s.checkpointQueued=false;s.failures=I("ai-failures",0);s.attempts=I("ai-attempts",0);s.segmentFailures=I("ai-segment-failures",0);s.segmentSuccesses=I("ai-segment-successes",0);s.triedMask=I("ai-tried-mask",0);s.errorBudget=budget();log::info("[ImixAI][BOOT] local geometry planner online; practice mode forced; adaptive budget={}",s.errorBudget);}
    s.time+=dt;s.actionTimer+=dt;s.scanTimer+=dt;s.telemetryTimer+=dt;s.checkpointTimer=std::max(0.f,s.checkpointTimer-dt);layer->m_isPracticeMode=true;
    if(s.checkpointQueued&&s.checkpointTimer<=0.f)s.checkpointQueued=false;

    if(layer->m_playerDied){release(layer);s.deathSeen=true;s.phase="WAITING PRACTICE RESPAWN";render();return;}
    if(s.deathSeen){s.deathSeen=false;s.stuckTimer=0.f;s.lastActionX=-100000.f;s.phase="RESCAN AFTER CHECKPOINT";scanLevel(p->getPositionX(),p->getPositionY());}

    float x=p->getPositionX(),y=p->getPositionY(),dx=x-s.lastX;s.speed=dt>.0001f?std::max(0.f,dx/dt):0.f;if(dx>.1f){s.hadProgress=true;s.stuckTimer=0.f;}else s.stuckTimer+=dt;s.lastX=x;s.lastY=y;
    if(!s.initialized){s.initialized=true;s.lastX=x;s.lastY=y;s.checkpointX=x;s.checkpointY=y;s.nextActionX=x+4.f;s.phase="INITIAL SCAN";scanLevel(x,y);searchLog(s.phase,x,y);}
    if(s.holding&&s.actionTimer>.085f)release(layer);
    if(s.scanTimer>.055f){s.scanTimer=0.f;scanLevel(x,y);}

    if(s.hadProgress&&x-s.checkpointX>95.f){queueCheckpoint(x-28.f,y);s.segmentSuccesses++;s.segmentFailures=0;s.errorBudget=budget();s.triedMask=0;SI("ai-segment-successes",s.segmentSuccesses);SI("ai-segment-failures",0);SI("ai-tried-mask",0);}

    if(!s.holding&&!s.deathSeen&&x+std::max(5.f,s.jumpLead)>=s.nextActionX&&x-s.lastActionX>10.f){s.phase="TESTING TRAJECTORY";tap(layer);}else if(s.searching){s.phase=(p->m_yVelocity>1.f)?"VERIFY ASCENT":(p->m_yVelocity<-1.f)?"VERIFY DESCENT":"ANALYZING TRAJECTORY";}else s.phase="OBSERVING";
    if(s.stuckTimer>.32f&&!s.holding){s.phase="RECOVERY INPUT";tap(layer);s.stuckTimer=0.f;}
    if(s.telemetryTimer>.30f){s.telemetryTimer=0.f;log::debug("[ImixAI][TELEMETRY] x={:.1f} y={:.1f} vy={:.1f} speed={:.1f} target={:.1f} gap={:.1f} candidate={} retry={}/{}",x,y,(float)p->m_yVelocity,s.speed,s.targetX,s.targetGap,candidateName(),s.segmentFailures,s.errorBudget);}
    render();SV("ai-last-x",x);SV("ai-last-y",y);
}
}
