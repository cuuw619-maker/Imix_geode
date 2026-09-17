#include "ImixMenu.hpp"

using namespace geode::prelude;

void ImixMenu::onVisualToggle(CCObject* sender) {
    int tag = static_cast<CCNode*>(sender)->getTag();
    const char* key = tag == 1 ? "ghost-player" : tag == 2 ? "hide-player" : "rainbow-player";
    bool current = Mod::get()->getSavedValue<bool>(key, false);
    Mod::get()->setSavedValue(key, !current);
    selectCategory(mSelectedCategory);
}

void ImixMenu::onPlayerScale(CCObject*) {
    int scale = Mod::get()->getSavedValue<int>("player-scale", 100);
    scale += 10;
    if (scale > 130) scale = 70;
    Mod::get()->setSavedValue("player-scale", scale);
    selectCategory(1);
}

void ImixMenu::onResetFeatures(CCObject*) {
    Mod::get()->setSavedValue("ghost-player", false);
    Mod::get()->setSavedValue("hide-player", false);
    Mod::get()->setSavedValue("rainbow-player", false);
    Mod::get()->setSavedValue("player-scale", 100);
    Mod::get()->setSavedValue("smart-startpos-captured", false);
    Mod::get()->setSavedValue("startpos-valid", false);
    Mod::get()->setSavedValue("startpos-request-capture", false);
    Mod::get()->setSavedValue("startpos-request-teleport", false);
    selectCategory(0);
}
