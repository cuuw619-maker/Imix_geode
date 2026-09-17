#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "ImixMenuModel.hpp"

class ImixMenu : public geode::Popup {
protected:
    bool init();

public:
    static ImixMenu* create();

    void onCategory(cocos2d::CCObject* sender);
    void onSmartToggle(cocos2d::CCObject* sender);
    void onSmartAction(cocos2d::CCObject* sender);
    void onVisualToggle(cocos2d::CCObject* sender);
    void onGameplayToggle(cocos2d::CCObject* sender);
    void onPlayerScale(cocos2d::CCObject* sender);
    void onAIToggle(cocos2d::CCObject* sender);
    void onAIReset(cocos2d::CCObject* sender);
    void onResetFeatures(cocos2d::CCObject* sender);

private:
    void selectCategory(int category);
    void renderCategory();
    void renderItem(const ImixMenuModel::Item& item, float y);
    void animateContentIn(int direction);
    void animateCategoryButtons();

    cocos2d::CCLayer* mContent = nullptr;
    cocos2d::CCArray* mCategoryButtons = nullptr;
    int mSelectedCategory = 0;
    int mPreviousCategory = 0;
    bool mAnimating = false;
};
