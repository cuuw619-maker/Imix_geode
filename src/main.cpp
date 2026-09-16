#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>

#include "ImixMenu.hpp"

using namespace geode::prelude;

static CCMenuItemSpriteExtra* createImixButton(CCObject* target, SEL_MenuHandler callback) {
    auto sprite = ButtonSprite::create("IMIX", "goldFont.fnt", "GJ_button_01.png", 1.f);
    sprite->setScale(0.68f);
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
            fallback->setPosition({this->getContentWidth() - 48.f, 38.f});
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

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();

        auto menu = this->getChildByID("left-button-menu");
        if (!menu) menu = this->getChildByID("right-button-menu");

        if (menu) {
            auto button = createImixButton(this, menu_selector(ImixPauseLayer::onImixPause));
            button->setID("imix-pause-button"_spr);
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto fallback = CCMenu::create();
            fallback->setPosition({48.f, 38.f});
            auto button = createImixButton(this, menu_selector(ImixPauseLayer::onImixPause));
            button->setID("imix-pause-button-fallback"_spr);
            fallback->addChild(button);
            fallback->updateLayout();
            this->addChild(fallback, 100);
        }
    }

    void onImixPause(CCObject*) {
        if (auto popup = ImixMenu::create()) popup->show();
    }
};
