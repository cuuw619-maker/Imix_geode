#include "ImixMenu.hpp"

using namespace geode::prelude;

bool ImixMenu::setup() {
    auto size = this->getContentSize();
    this->setTitle("Imix");

    auto background = CCLayerColor::create({25, 25, 30, 255});
    background->setContentSize(size);
    background->setPosition(0, 0);
    this->addChild(background, -1);

    constexpr float sidebarWidth = 115.f;
    constexpr float margin = 12.f;
    constexpr float top = 42.f;
    constexpr float bottom = 12.f;

    auto sidebar = CCLayerColor::create({35, 35, 42, 255});
    sidebar->setContentSize({sidebarWidth, size.height - top - bottom});
    sidebar->setPosition({margin, bottom});
    this->addChild(sidebar);

    mContent = CCLayer::create();
    mContent->setContentSize({size.width - sidebarWidth - margin * 3, size.height - top - bottom});
    mContent->setPosition({sidebarWidth + margin * 2, bottom});
    this->addChild(mContent);

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    const char* categories[] = {"Smart StartPos", "Test 1", "Test 2"};
    for (int i = 0; i < 3; ++i) {
        auto label = CCLabelBMFont::create(categories[i], "goldFont.fnt");
        label->setScale(i == 0 ? 0.34f : 0.42f);
        auto button = CCMenuItemLabel::create(label, this, menu_selector(ImixMenu::onCategory));
        button->setTag(i);
        button->setContentSize({sidebarWidth - 16.f, 34.f});
        button->setPosition({sidebarWidth / 2.f, sidebar->getContentHeight() - 22.f - i * 40.f});
        sidebar->addChild(button);
        mCategoryButtons->addObject(button);
    }

    this->selectCategory(0);
    return true;
}

void ImixMenu::onCategory(CCObject* sender) {
    auto button = static_cast<CCNode*>(sender);
    this->selectCategory(button->getTag());
}

void ImixMenu::selectCategory(int category) {
    mSelectedCategory = category;

    if (mContent) {
        mContent->removeAllChildren();
    }

    if (category == 0) {
        auto label = CCLabelBMFont::create("Smart StartPos", "goldFont.fnt");
        label->setScale(0.55f);
        label->setPosition({mContent->getContentWidth() / 2.f, mContent->getContentHeight() - 28.f});
        mContent->addChild(label);
    }

    if (mCategoryButtons) {
        for (unsigned int i = 0; i < mCategoryButtons->count(); ++i) {
            auto button = static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));
            button->setColor(i == static_cast<unsigned int>(mSelectedCategory) ? ccColor3B{255, 215, 70} : ccColor3B{200, 200, 200});
        }
    }
}
