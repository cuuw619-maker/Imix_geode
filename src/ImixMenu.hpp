#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

class ImixMenu : public geode::Popup {
protected:
    bool init();

private:
    void onCategory(cocos2d::CCObject* sender);
    void selectCategory(int category);
    cocos2d::CCLayer* mContent = nullptr;
    cocos2d::CCArray* mCategoryButtons = nullptr;
    int mSelectedCategory = 0;

public:
    static ImixMenu* create();
};
