#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <algorithm>
#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
CCLabelBMFont* gdText(const char* text) {
    auto x = CCLabelBMFont::create(text, "bigFont.fnt");
    x->setScale(.55f);
    return x;
}

CCMenuItemSpriteExtra* imixButton(CCObject* target, SEL_MenuHandler cb) {
    // Real GD button texture, not a floating custom widget.
    auto bg = CCScale9Sprite::create("GJ_button_01.png");
    if (!bg) return nullptr;
    bg->setContentSize({112.f, 38.f});
    auto text = gdText("IMIX");
    text->setPosition({56.f, 19.f});
    bg->addChild(text);
    auto item = CCMenuItemSpriteExtra::create(bg, target, cb);
    item->setContentSize({112.f, 38.f});
    return item;
}

void pressAnim(CCNode* node) {
    if (!node) return;
    node->stopAllActions();
    node->setScale(.96f);
    node->runAction(CCEaseSineOut::create(CCScaleTo::create(.14f, 1.f)));
}

void openImix(CCNode* button) {
    pressAnim(button);
    auto popup = ImixMenu::create();
    if (!popup) return;

    // Popup::init handles the normal GD/Geode centering. ImixMenu only
    // applies a bounded scale so it cannot leave the screen on small phones.
    auto win = CCDirector::sharedDirector()->getWinSize();
    const auto size = popup->getContentSize() * popup->getScale();
    const float margin = std::max(8.f, std::min(win.width, win.height) * .02f);
    auto pos = popup->getPosition();
    pos.x = std::clamp(pos.x, size.width * .5f + margin, win.width - size.width * .5f - margin);
    pos.y = std::clamp(pos.y, size.height * .5f + margin, win.height - size.height * .5f - margin);
    popup->setPosition(pos);

    auto finalScale = popup->getScale();
    popup->setScale(finalScale * .94f);
    popup->runAction(CCEaseBackOut::create(CCScaleTo::create(.18f, finalScale)));
}
}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        auto menu = CCMenu::create();
        menu->setID("imix-main-menu");
        menu->setPosition({64.f, 34.f});
        if (auto b = imixButton(this, menu_selector(ImixMenuLayer::onImix))) {
            b->setPosition({0, 0});
            menu->addChild(b);
        }
        addChild(menu, 1000);
        return true;
    }

    void onImix(CCObject* sender) {
        openImix(static_cast<CCNode*>(sender));
    }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = CCMenu::create();
        menu->setID("imix-pause-menu");
        menu->setPosition({64.f, 34.f});
        if (auto b = imixButton(this, menu_selector(ImixPauseLayer::onImixPause))) {
            b->setPosition({0, 0});
            menu->addChild(b);
        }
        addChild(menu, 1000);
    }

    void onImixPause(CCObject* sender) {
        openImix(static_cast<CCNode*>(sender));
    }
};
