#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>

#include "ImixMenu.hpp"

using namespace geode::prelude;

static CCMenuItemSpriteExtra* createImixButton(CCObject* target, SEL_MenuHandler callback) {
    auto sprite = ButtonSprite::create("IMIX", "goldFont.fnt", "GJ_button_01.png", 1.f);
    sprite->setScale(0.72f);
    return CCMenuItemSpriteExtra::create(sprite, target, callback);
}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (menu) {
            auto button = createImixButton(this, menu_selector(ImixMenuLayer::onImix));
            button->setID("imix-button"_spr);
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto fallback = CCMenu::create();
            fallback->setPosition({this->getContentWidth() - 55.f, 42.f});
            auto button = createImixButton(this, menu_selector(ImixMenuLayer::onImix));
            button->setID("imix-button-fallback"_spr);
            fallback->addChild(button);
            fallback->updateLayout();
            this->addChild(fallback, 100);
        }
        return true;
    }

    void onImix(CCObject*) {
        if (auto popup = ImixMenu::create()) popup->show();
    }
};
