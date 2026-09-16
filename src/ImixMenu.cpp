#include "ImixMenu.hpp"

using namespace geode::prelude;

bool ImixMenu::init() {
    if (!Popup::init(520.f, 320.f, "GJ_square01.png")) {
        return false;
    }

    this->setTitle("Imix");

    auto size = m_mainLayer->getContentSize();
    constexpr float sidebarWidth = 125.f;
    constexpr float margin = 12.f;

    auto background = CCLayerColor::create({25, 25, 30, 255});
    background->setContentSize(size);
    background->setPosition(0, 0);
    m_mainLayer->addChildAtPosition(background, Anchor::Center);

    auto sidebar = CCLayerColor::create({35, 35, 42, 255});
    sidebar->setContentSize({sidebarWidth, size.height - margin * 2});
    sidebar->setPosition({margin, margin});
    m_mainLayer->addChild(sidebar);

    mContent = CCLayer::create();
    mContent->setContentSize({size.width - sidebarWidth - margin * 3, size.height - margin * 2});
    mContent->setPosition({sidebarWidth + margin * 2, margin});
    m_mainLayer->addChild(mContent);

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    auto categoryMenu = CCMenu::create();
    categoryMenu->setPosition({sidebarWidth / 2.f, sidebar->getContentHeight() - 30.f});
    categoryMenu->setContentSize({sidebarWidth, sidebar->getContentHeight() - 10.f});
    sidebar->addChild(categoryMenu);

    const char* categories[] = {"Smart StartPos", "Player", "Visual"};
    for (int i = 0; i < 3; ++i) {
        auto label = CCLabelBMFont::create(categories[i], "goldFont.fnt");
        label->setScale(i == 0 ? 0.31f : 0.42f);
        auto button = CCMenuItemLabel::create(label, this, menu_selector(ImixMenu::onCategory));
        button->setTag(i);
        button->setContentSize({sidebarWidth - 14.f, 38.f});
        button->setPosition({0.f, -i * 46.f});
        categoryMenu->addChild(button);
        mCategoryButtons->addObject(button);
    }

    this->selectCategory(0);
    return true;
}

ImixMenu* ImixMenu::create() {
    auto ret = new ImixMenu();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
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

    const char* titles[] = {"Smart StartPos", "Player", "Visual"};
    auto title = CCLabelBMFont::create(titles[category], "goldFont.fnt");
    title->setScale(0.55f);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({16.f, mContent->getContentHeight() - 25.f});
    mContent->addChild(title);

    auto subtitle = CCLabelBMFont::create(
        category == 0 ? "Quick access and spawn position tools" :
        category == 1 ? "Player-related Imix options" :
                         "Visual and interface options",
        "chatFont.fnt"
    );
    subtitle->setScale(0.55f);
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({16.f, mContent->getContentHeight() - 48.f});
    mContent->addChild(subtitle);

    if (category == 0) {
        auto card = CCLayerColor::create({42, 42, 50, 255});
        card->setContentSize({mContent->getContentWidth() - 32.f, 82.f});
        card->setPosition({16.f, mContent->getContentHeight() - 145.f});
        mContent->addChild(card);

        auto label = CCLabelBMFont::create("Smart StartPos", "bigFont.fnt");
        label->setScale(0.38f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({14.f, 55.f});
        card->addChild(label);

        auto status = CCLabelBMFont::create("Base module ready", "chatFont.fnt");
        status->setScale(0.55f);
        status->setAnchorPoint({0.f, 0.5f});
        status->setPosition({14.f, 28.f});
        card->addChild(status);
    }

    if (category == 1) {
        auto label = CCLabelBMFont::create("Player settings will be added here.", "chatFont.fnt");
        label->setScale(0.6f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({16.f, mContent->getContentHeight() - 95.f});
        mContent->addChild(label);
    }

    if (category == 2) {
        auto label = CCLabelBMFont::create("Visual settings will be added here.", "chatFont.fnt");
        label->setScale(0.6f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition({16.f, mContent->getContentHeight() - 95.f});
        mContent->addChild(label);
    }

    if (mCategoryButtons) {
        for (unsigned int i = 0; i < mCategoryButtons->count(); ++i) {
            auto button = static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));
            button->setColor(i == static_cast<unsigned int>(mSelectedCategory)
                ? ccColor3B{255, 215, 70}
                : ccColor3B{200, 200, 200});
        }
    }
}
