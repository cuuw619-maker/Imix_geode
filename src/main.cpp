#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>

#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLayer* floatingVisual() {
    constexpr float S = 56.f;
    auto root = CCLayer::create();
    root->setContentSize({S, S});
    root->setAnchorPoint({.5f, .5f});

    auto d = CCDrawNode::create();
    d->setContentSize({S, S});
    d->drawCircle({S*.5f, S*.5f}, 25.f, {0.035f,0.055f,0.085f,.98f}, 0, 2.f, {0.20f,0.48f,0.72f,.95f});
    d->drawCircle({S*.5f, S*.5f}, 20.f, {0.08f,0.13f,0.19f,1.f}, 0, 0, {0,0,0,0});
    root->addChild(d);

    auto label = CCLabelTTF::create("I", "sans-serif", 20.f);
    label->setColor({235,245,255});
    label->setPosition({S*.5f,S*.5f});
    root->addChild(label, 2);
    return root;
}

CCMenuItemSpriteExtra* createFloatingButton(CCObject* target, SEL_MenuHandler callback) {
    auto item = CCMenuItemSpriteExtra::create(floatingVisual(), target, callback);
    item->setContentSize({56.f,56.f});
    item->setAnchorPoint({.5f,.5f});
    return item;
}

CCMenu* makeFloatingMenu(CCObject* target, SEL_MenuHandler callback, const char* id, CCPoint pos) {
    auto menu = CCMenu::create();
    menu->setID(id);
    menu->setAnchorPoint({.5f,.5f});
    menu->setPosition(pos);
    auto button = createFloatingButton(target, callback);
    button->setPosition({0,0});
    menu->addChild(button);
    return menu;
}

void animateButton(CCNode* node) {
    if (!node) return;
    node->stopAllActions();
    node->setScale(.93f);
    node->runAction(CCEaseSineOut::create(CCScaleTo::create(.14f,1.f)));
}

void openImix(CCNode* button) {
    if (!button) return;
    animateButton(button);
    auto popup = ImixMenu::create();
    if (!popup) return;

    auto win = CCDirector::sharedDirector()->getWinSize();
    auto parent = button->getParent();
    auto origin = parent->convertToWorldSpace(button->getPosition());

    popup->show();
    popup->setPosition(origin);
    popup->setScale(.08f);
    popup->runAction(CCSpawn::create(
        CCEaseBackOut::create(CCScaleTo::create(.28f,1.f)),
        CCEaseSineOut::create(CCMoveTo::create(.28f,{win.width*.5f,win.height*.5f})),
        nullptr
    ));
}
}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto menu = makeFloatingMenu(this, menu_selector(ImixMenuLayer::onImix), "imix-main-menu", {win.width-40.f,42.f});
        this->addChild(menu,10000);
        return true;
    }

    void onImix(CCObject*) {
        if (auto menu = this->getChildByID("imix-main-menu")) openImix(menu);
    }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto menu = makeFloatingMenu(this, menu_selector(ImixPauseLayer::onImixPause), "imix-pause-menu", {win.width-40.f,42.f});
        this->addChild(menu,10000);
    }

    void onImixPause(CCObject*) {
        if (auto menu = this->getChildByID("imix-pause-menu")) openImix(menu);
    }
};
