#pragma once

#include <Geode/Geode.hpp>

class ImixMenu : public geode::Popup<> {
protected:
    bool setup() override;

private:
    void onCategory(CCObject* sender);
    void selectCategory(int category);
    cocos2d::CCLayer* mContent = nullptr;
    cocos2d::CCArray* mCategoryButtons = nullptr;
    int mSelectedCategory = 0;
};
