#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <vector>
#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLayer* roundedButton(float w, float h, const char* text) {
    auto layer = CCLayer::create();
    layer->setContentSize({w, h});
    layer->setAnchorPoint({0.5f, 0.5f});

    auto draw = CCDrawNode::create();
    draw->setContentSize({w, h});
    std::vector<CCPoint> pts;
    const float r = std::min(8.f, std::min(w, h) * .5f);
    const int n = 8;
    const float pi = 3.14159265359f;
    const float cx[4] = {r, w-r, w-r, r};
    const float cy[4] = {r, r, h-r, h-r};
    const float st[4] = {pi, pi*1.5f, 0.f, pi*.5f};
    for (int c=0;c<4;c++) for(int i=0;i<=n;i++) {
        float a=st[c]+pi*.5f*(static_cast<float>(i)/n);
        pts.push_back({cx[c]+std::cos(a)*r,cy[c]+std::sin(a)*r});
    }
    draw->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), {0.12f,0.16f,0.21f,1.f}, 0.f, {0.12f,0.16f,0.21f,1.f});
    layer->addChild(draw, 0);

    auto label = CCLabelTTF::create(text, "sans-serif", 11.f);
    label->setColor({238,242,248});
    label->setPosition({w*.5f,h*.5f});
    layer->addChild(label, 1);
    return layer;
}

CCMenuItemSpriteExtra* createImixButton(CCObject* target, SEL_MenuHandler callback) {
    auto visual = roundedButton(76.f, 34.f, "IMIX");
    auto item = CCMenuItemSpriteExtra::create(visual, target, callback);
    // The menu item and visual use the exact same dimensions, so the touch box
    // cannot drift away from the painted button.
    item->setContentSize({76.f,34.f});
    item->setAnchorPoint({0.5f,0.5f});
    return item;
}

CCMenu* makeFixedMenu(float x, float y, CCObject* target, SEL_MenuHandler callback, const char* id) {
    auto menu = CCMenu::create();
    menu->setID(id);
    menu->setPosition({x,y});
    auto button = createImixButton(target, callback);
    button->setPosition({0,0});
    menu->addChild(button);
    return menu;
}
}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        // Do not insert into GD's auto-layout menu: its layout can move the
        // sprite independently of the intended touch geometry. Imix owns its
        // own fixed overlay layer instead.
        auto size = this->getContentSize();
        auto menu = makeFixedMenu(size.width - 52.f, 40.f, this, menu_selector(ImixMenuLayer::onImix), "imix-main-menu");
        this->addChild(menu, 1000);
        return true;
    }
    void onImix(CCObject*) {
        if (auto popup=ImixMenu::create()) popup->show();
    }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        // Same rule for pause: never let a GD layout pass relocate the custom
        // sprite while the original touch target remains elsewhere.
        auto size = this->getContentSize();
        auto menu = makeFixedMenu(52.f, 42.f, this, menu_selector(ImixPauseLayer::onImixPause), "imix-pause-menu");
        this->addChild(menu, 1000);
    }
    void onImixPause(CCObject*) {
        if (auto popup=ImixMenu::create()) popup->show();
    }
};
