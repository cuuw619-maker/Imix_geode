#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <vector>

#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLayer* floatingVisual() {
    constexpr float S = 56.f;
    auto root = CCLayer::create();
    root->setContentSize({S,S});
    root->setAnchorPoint({.5f,.5f});

    auto d = CCDrawNode::create();
    d->setContentSize({S,S});
    std::vector<CCPoint> pts;
    const float r=17.f, pi=3.14159265359f;
    const float cx[4]={r,S-r,S-r,r},cy[4]={r,r,S-r,S-r},st[4]={pi,pi*1.5f,0.f,pi*.5f};
    for(int c=0;c<4;c++)for(int i=0;i<=10;i++){float a=st[c]+pi*.5f*(float(i)/10.f);pts.push_back({cx[c]+std::cos(a)*r,cy[c]+std::sin(a)*r});}
    d->drawPolygon(pts.data(),static_cast<unsigned>(pts.size()),{.035f,.055f,.085f,.98f},2.f,{.20f,.48f,.72f,.95f});
    root->addChild(d);

    auto inner=CCDrawNode::create();
    std::vector<CCPoint> in;
    const float ir=13.f;
    const float icx[4]={ir,S-ir,S-ir,ir},icy[4]={ir,ir,S-ir,S-ir};
    for(int c=0;c<4;c++)for(int i=0;i<=10;i++){float a=st[c]+pi*.5f*(float(i)/10.f);in.push_back({icx[c]+std::cos(a)*ir,icy[c]+std::sin(a)*ir});}
    inner->drawPolygon(in.data(),static_cast<unsigned>(in.size()),{.08f,.13f,.19f,1.f},0.f,{0,0,0,0});
    root->addChild(inner,1);

    auto label=CCLabelTTF::create("I","sans-serif",20.f);
    label->setColor({235,245,255});label->setPosition({S*.5f,S*.5f});root->addChild(label,2);
    return root;
}

CCMenuItemSpriteExtra* createFloatingButton(CCObject* target, SEL_MenuHandler callback){
    auto item=CCMenuItemSpriteExtra::create(floatingVisual(),target,callback);
    item->setContentSize({56.f,56.f});item->setAnchorPoint({.5f,.5f});return item;
}

CCMenu* makeFloatingMenu(CCObject* target, SEL_MenuHandler callback,const char* id,CCPoint pos){
    auto menu=CCMenu::create();menu->setID(id);menu->setAnchorPoint({.5f,.5f});menu->setPosition(pos);
    auto button=createFloatingButton(target,callback);button->setPosition({0,0});menu->addChild(button);return menu;
}

void animateButton(CCNode* node){if(!node)return;node->stopAllActions();node->setScale(.93f);node->runAction(CCEaseSineOut::create(CCScaleTo::create(.14f,1.f)));}

void openImix(CCNode* button){
    if(!button)return;animateButton(button);auto popup=ImixMenu::create();if(!popup)return;
    auto win=CCDirector::sharedDirector()->getWinSize();auto parent=button->getParent();auto origin=parent->convertToWorldSpace(button->getPosition());
    popup->show();popup->setPosition(origin);popup->setScale(.08f);
    popup->runAction(CCSpawn::create(CCEaseBackOut::create(CCScaleTo::create(.28f,1.f)),CCEaseSineOut::create(CCMoveTo::create(.28f,{win.width*.5f,win.height*.5f})),nullptr));
}
}

class $modify(ImixMenuLayer,MenuLayer){
public:
    bool init(){if(!MenuLayer::init())return false;auto win=CCDirector::sharedDirector()->getWinSize();auto menu=makeFloatingMenu(this,menu_selector(ImixMenuLayer::onImix),"imix-main-menu",{win.width-40.f,42.f});this->addChild(menu,10000);return true;}
    void onImix(CCObject*){if(auto menu=this->getChildByID("imix-main-menu"))openImix(menu);}
};

class $modify(ImixPauseLayer,PauseLayer){
public:
    void customSetup(){PauseLayer::customSetup();auto win=CCDirector::sharedDirector()->getWinSize();auto menu=makeFloatingMenu(this,menu_selector(ImixPauseLayer::onImixPause),"imix-pause-menu",{win.width-40.f,42.f});this->addChild(menu,10000);}
    void onImixPause(CCObject*){if(auto menu=this->getChildByID("imix-pause-menu"))openImix(menu);}
};
