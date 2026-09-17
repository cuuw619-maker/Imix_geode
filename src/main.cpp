#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <vector>
#include <algorithm>

#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLayer* floatingVisual() {
    constexpr float S = 48.f;
    auto root = CCLayer::create();
    root->setContentSize({S,S}); root->setAnchorPoint({.5f,.5f});
    auto d=CCDrawNode::create(); d->setContentSize({S,S});
    std::vector<CCPoint> p; const float r=14.f,pi=3.14159265359f;
    const float cx[4]={r,S-r,S-r,r},cy[4]={r,r,S-r,S-r},st[4]={pi,pi*1.5f,0.f,pi*.5f};
    for(int c=0;c<4;c++)for(int i=0;i<=10;i++){float a=st[c]+pi*.5f*(float(i)/10.f);p.push_back({cx[c]+std::cos(a)*r,cy[c]+std::sin(a)*r});}
    d->drawPolygon(p.data(),(unsigned)p.size(),{.035f,.055f,.085f,.98f},2.f,{.20f,.48f,.72f,.95f}); root->addChild(d);
    auto label=CCLabelTTF::create("I","sans-serif",18.f); label->setColor({235,245,255}); label->setPosition({S*.5f,S*.5f}); root->addChild(label,2);
    return root;
}
CCMenuItemSpriteExtra* createFloatingButton(CCObject* target,SEL_MenuHandler cb){auto x=CCMenuItemSpriteExtra::create(floatingVisual(),target,cb);x->setContentSize({48,48});x->setAnchorPoint({.5f,.5f});return x;}
CCMenu* makeFloatingMenu(CCObject* target,SEL_MenuHandler cb,const char* id,CCPoint pos){auto m=CCMenu::create();m->setID(id);m->setPosition(pos);auto b=createFloatingButton(target,cb);b->setPosition({0,0});m->addChild(b);return m;}
void animateButton(CCNode* n){if(!n)return;n->stopAllActions();n->setScale(.94f);n->runAction(CCEaseSineOut::create(CCScaleTo::create(.16f,1.f)));}
void openImix(CCNode* button){
    if(!button)return; animateButton(button); auto popup=ImixMenu::create(); if(!popup)return;
    auto win=CCDirector::sharedDirector()->getWinSize(); auto parent=button->getParent();
    auto origin=parent->convertToWorldSpace(button->getPosition());
    const float responsive=std::clamp(std::min(win.width/1920.f,win.height/1080.f),.72f,1.f);
    popup->show(); popup->setPosition(origin); popup->setScale(.72f*responsive);
    popup->runAction(CCSpawn::create(
        CCEaseBackOut::create(CCScaleTo::create(.30f,responsive)),
        CCEaseSineOut::create(CCMoveTo::create(.30f,{win.width*.5f,win.height*.5f})),nullptr));
}
}

class $modify(ImixMenuLayer,MenuLayer){
public:
    bool init(){if(!MenuLayer::init())return false;auto w=CCDirector::sharedDirector()->getWinSize();auto m=makeFloatingMenu(this,menu_selector(ImixMenuLayer::onImix),"imix-main-menu",{w.width-30.f,30.f});addChild(m,10000);return true;}
    void onImix(CCObject*){if(auto m=getChildByID("imix-main-menu"))openImix(m);}
};
class $modify(ImixPauseLayer,PauseLayer){
public:
    void customSetup(){PauseLayer::customSetup();auto w=CCDirector::sharedDirector()->getWinSize();auto m=makeFloatingMenu(this,menu_selector(ImixPauseLayer::onImixPause),"imix-pause-menu",{w.width-30.f,30.f});addChild(m,10000);}
    void onImixPause(CCObject*){if(auto m=getChildByID("imix-pause-menu"))openImix(m);}
};
