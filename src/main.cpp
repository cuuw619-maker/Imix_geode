#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include "ImixMenu.hpp"

using namespace geode::prelude;

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) {
            return true;
        }

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) {
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(ImixMenuLayer::onImix)
        );
        button->setID("imix-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onImix(CCObject*) {
        ImixMenu::create()->show();
    }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    bool init(bool unfocused) {
        if (!PauseLayer::init(unfocused)) {
            return false;
        }

        auto menu = this->getChildByID("left-button-menu");
        if (!menu) {
            menu = this->getChildByID("right-button-menu");
        }
        if (!menu) {
            return true;
        }

        auto sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!sprite) {
            return true;
        }

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(ImixPauseLayer::onImix)
        );
        button->setID("imix-button"_spr);
        menu->addChild(button);
        menu->updateLayout();
        return true;
    }

    void onImix(CCObject*) {
        ImixMenu::create()->show();
    }
};
