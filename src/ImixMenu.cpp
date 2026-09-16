#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
    constexpr ccColor3B BG{18, 20, 25};
    constexpr ccColor3B PANEL{27, 30, 37};
    constexpr ccColor3B PANEL_ACTIVE{45, 49, 59};
    constexpr ccColor3B TEXT{235, 238, 244};
    constexpr ccColor3B MUTED{145, 151, 164};
    constexpr ccColor3B ACCENT{105, 190, 255};
}

static CCLabelTTF* text(const char* value, float size, ccColor3B color) {
    auto label = CCLabelTTF::create(value, "sans-serif", size);
    label->setColor(color);
    return label;
}

bool ImixMenu::init() {
    if (!Popup::init(620.f, 390.f, "GJ_square01.png")) return false;

    auto size = m_mainLayer->getContentSize();
    auto root = CCLayerColor::create({BG.r, BG.g, BG.b, 255});
    root->setContentSize(size);
    root->setPosition({0.f, 0.f});
    m_mainLayer->addChild(root, 100);

    auto header = CCLayerColor::create({23, 26, 32, 255});
    header->setContentSize({size.width, 70.f});
    header->setPosition({0.f, size.height - 70.f});
    root->addChild(header);

    auto title = text("IMIX", 25.f, TEXT);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({28.f, 43.f});
    header->addChild(title);

    auto version = text("MOD CONTROL CENTER", 11.f, MUTED);
    version->setAnchorPoint({0.f, 0.5f});
    version->setPosition({29.f, 20.f});
    header->addChild(version);

    constexpr float sidebarWidth = 170.f;
    auto sidebar = CCLayerColor::create({22, 24, 29, 255});
    sidebar->setContentSize({sidebarWidth, size.height - 70.f});
    sidebar->setPosition({0.f, 0.f});
    root->addChild(sidebar);

    mContent = CCLayer::create();
    mContent->setContentSize({size.width - sidebarWidth, size.height - 70.f});
    mContent->setPosition({sidebarWidth, 0.f});
    root->addChild(mContent);

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    auto categoryMenu = CCMenu::create();
    categoryMenu->setPosition({sidebarWidth / 2.f, sidebar->getContentHeight() - 45.f});
    sidebar->addChild(categoryMenu);

    const char* categories[] = {"Smart StartPos", "Player", "Visual"};
    for (int i = 0; i < 3; ++i) {
        auto label = text(categories[i], 15.f, TEXT);
        label->setAnchorPoint({0.f, 0.5f});
        auto button = CCMenuItemLabel::create(label, this, menu_selector(ImixMenu::onCategory));
        button->setTag(i);
        button->setContentSize({sidebarWidth - 28.f, 46.f});
        button->setPosition({0.f, -i * 58.f});
        categoryMenu->addChild(button);
        mCategoryButtons->addObject(button);
    }

    auto footer = text("Imix 1.0.0", 10.f, MUTED);
    footer->setAnchorPoint({0.f, 0.f});
    footer->setPosition({20.f, 16.f});
    sidebar->addChild(footer);

    selectCategory(0);
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
    selectCategory(static_cast<CCNode*>(sender)->getTag());
}

void ImixMenu::selectCategory(int category) {
    mSelectedCategory = category;
    if (!mContent) return;
    mContent->removeAllChildren();

    const char* titles[] = {"Smart StartPos", "Player", "Visual"};
    const char* subtitles[] = {
        "Fast access to spawn and level tools",
        "Player-related Imix options",
        "Interface and visual options"
    };

    auto title = text(titles[category], 23.f, TEXT);
    title->setAnchorPoint({0.f, 1.f});
    title->setPosition({28.f, mContent->getContentHeight() - 28.f});
    mContent->addChild(title);

    auto subtitle = text(subtitles[category], 12.f, MUTED);
    subtitle->setAnchorPoint({0.f, 1.f});
    subtitle->setPosition({29.f, mContent->getContentHeight() - 58.f});
    mContent->addChild(subtitle);

    auto card = CCLayerColor::create({PANEL.r, PANEL.g, PANEL.b, 255});
    card->setContentSize({mContent->getContentWidth() - 56.f, 108.f});
    card->setPosition({28.f, mContent->getContentHeight() - 190.f});
    mContent->addChild(card);

    if (category == 0) {
        auto label = text("Smart StartPos", 17.f, TEXT);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({18.f, 82.f});
        card->addChild(label);

        auto status = text("Base module ready", 12.f, MUTED);
        status->setAnchorPoint({0.f, 1.f});
        status->setPosition({18.f, 56.f});
        card->addChild(status);

        auto state = text("READY", 11.f, ACCENT);
        state->setAnchorPoint({1.f, 0.5f});
        state->setPosition({card->getContentWidth() - 18.f, 28.f});
        card->addChild(state);
    } else if (category == 1) {
        auto label = text("Player settings", 17.f, TEXT);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({18.f, 82.f});
        card->addChild(label);

        auto status = text("No player options configured yet", 12.f, MUTED);
        status->setAnchorPoint({0.f, 1.f});
        status->setPosition({18.f, 55.f});
        card->addChild(status);
    } else {
        auto label = text("Visual settings", 17.f, TEXT);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({18.f, 82.f});
        card->addChild(label);

        auto status = text("Custom Imix interface is active", 12.f, MUTED);
        status->setAnchorPoint({0.f, 1.f});
        status->setPosition({18.f, 55.f});
        card->addChild(status);
    }

    if (mCategoryButtons) {
        for (unsigned int i = 0; i < mCategoryButtons->count(); ++i) {
            auto button = static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));
            button->setColor(i == static_cast<unsigned int>(mSelectedCategory) ? ACCENT : TEXT);
        }
    }
}
