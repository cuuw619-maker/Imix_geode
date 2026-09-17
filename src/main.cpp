#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLayer* roundedButton(float w, float h, const char* text) {
    auto layer = CCLayer::create();
    layer->setContentSize({w, h});
    auto draw = CCDrawNode::create();
    std::vector<CCPoint> pts;
    const float r = 8.f;
    const int n = 6;
    const float pi = 3.14159265359f;
    const float cx[4] = {r, w-r, w-r, r};
    const float cy[4] = {r, r, h-r, h-r};
    const float st[4] = {pi, pi*1.5f, 0.f, pi*.5f};
    for (int c=0;c<4;c++) for(int i=0;i<=n;i++) {
        float a=st[c]+pi*.5f*(static_cast<float>(i)/n);
        pts.push_back({cx[c]+std::cos(a)*r,cy[c]+std::sin(a)*r});
    }
    draw->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), {0.12f,0.16f,0.21f,1.f}, 0.f, {0.12f,0.16f,0.21f,1.f});
    layer->addChild(draw);
    auto label = CCLabelTTF::create(text, "sans-serif", 11.f);
    label->setColor({238,242,248});
    label->setPosition({w*.5f,h*.5f});
    layer->addChild(label);
    return layer;
}
CCMenuItemSpriteExtra* createImixButton(CCObject* target, SEL_MenuHandler callback) {
    auto sprite = roundedButton(70.f, 30.f, "IMIX");
    return CCMenuItemSpriteExtra::create(sprite, target, callback);
}
}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        auto menu = this->getChildByID("bottom-menu");
        if (menu) {
            auto button=createImixButton(this,menu_selector(ImixMenuLayer::onImix));
            button->setID("imix-button"); menu->addChild(button); menu->updateLayout();
        } else {
            auto fallback=CCMenu::create();fallback->setPosition({this->getContentWidth()-48.f,38.f});
            auto button=createImixButton(this,menu_selector(ImixMenuLayer::onImix));button->setID("imix-button-fallback");fallback->addChild(button);this->addChild(fallback,100);
        }
        return true;
    }
    void onImix(CCObject*) { if(auto popup=ImixMenu::create()) popup->show(); }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        auto menu=this->getChildByID("left-button-menu");
        if(!menu) menu=this->getChildByID("right-button-menu");
        if(menu){auto button=createImixButton(this,menu_selector(ImixPauseLayer::onImixPause));button->setID("imix-pause-button");menu->addChild(button);menu->updateLayout();}
        else {auto fallback=CCMenu::create();fallback->setPosition({48.f,38.f});auto button=createImixButton(this,menu_selector(ImixPauseLayer::onImixPause));button->setID("imix-pause-button-fallback");fallback->addChild(button);this->addChild(fallback,100);}
    }
    void onImixPause(CCObject*) { if(auto popup=ImixMenu::create()) popup->show(); }
};
