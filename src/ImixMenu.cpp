#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {
    constexpr ccColor3B BG{14, 17, 22};
    constexpr ccColor3B HEADER{19, 23, 29};
    constexpr ccColor3B SIDEBAR{17, 20, 26};
    constexpr ccColor3B PANEL{25, 30, 38};
    constexpr ccColor3B TEXT{238, 241, 247};
    constexpr ccColor3B MUTED{139, 148, 162};
    constexpr ccColor3B ACCENT{105, 190, 255};
    constexpr ccColor3B SUCCESS{111, 220, 157};
}

static CCLabelTTF* text(const char* value, float size, ccColor3B color) {
    auto label = CCLabelTTF::create(value, "sans-serif", size);
    label->setColor(color);
    return label;
}

static CCMenuItemLabel* makeTextButton(const char* label, CCObject* target, SEL_MenuHandler callback, int tag = 0) {
    auto item = CCMenuItemLabel::create(text(label, 14.f, TEXT), target, callback);
    item->setTag(tag);
    return item;
}

bool ImixMenu::init() {
    if (!Popup::init(500.f, 315.f, "GJ_square01.png")) return false;

    // Fit the logical UI to the actual viewport instead of using one huge fixed size.
    auto win = CCDirector::sharedDirector()->getWinSize();
    auto popupSize = m_mainLayer->getContentSize();
    auto fitX = (win.width * 0.86f) / popupSize.width;
    auto fitY = (win.height * 0.72f) / popupSize.height;
    auto fit = std::min(1.0f, std::min(fitX, fitY));
    this->setScale(fit);

    auto root = CCLayerColor::create({BG.r, BG.g, BG.b, 255});
    root->setContentSize(popupSize);
    root->setPosition({0.f, 0.f});
    m_mainLayer->addChild(root, 100);

    auto header = CCLayerColor::create({HEADER.r, HEADER.g, HEADER.b, 255});
    header->setContentSize({popupSize.width, 58.f});
    header->setPosition({0.f, popupSize.height - 58.f});
    root->addChild(header);

    auto title = text("IMIX", 22.f, TEXT);
    title->setAnchorPoint({0.f, 0.5f});
    title->setPosition({22.f, 36.f});
    header->addChild(title);

    auto subtitle = text("MOD CONTROL CENTER", 9.f, MUTED);
    subtitle->setAnchorPoint({0.f, 0.5f});
    subtitle->setPosition({23.f, 17.f});
    header->addChild(subtitle);

    auto status = text("ONLINE", 9.f, SUCCESS);
    status->setAnchorPoint({1.f, 0.5f});
    status->setPosition({popupSize.width - 22.f, 29.f});
    header->addChild(status);

    constexpr float sidebarWidth = 142.f;
    auto sidebar = CCLayerColor::create({SIDEBAR.r, SIDEBAR.g, SIDEBAR.b, 255});
    sidebar->setContentSize({sidebarWidth, popupSize.height - 58.f});
    sidebar->setPosition({0.f, 0.f});
    root->addChild(sidebar);

    mContent = CCLayer::create();
    mContent->setContentSize({popupSize.width - sidebarWidth, popupSize.height - 58.f});
    mContent->setPosition({sidebarWidth, 0.f});
    root->addChild(mContent);

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    auto categoryMenu = CCMenu::create();
    categoryMenu->setPosition({sidebarWidth / 2.f, sidebar->getContentHeight() - 92.f});
    sidebar->addChild(categoryMenu);

    const char* categories[] = {"Smart StartPos", "Player", "Visual"};
    for (int i = 0; i < 3; ++i) {
        auto button = makeTextButton(categories[i], this, menu_selector(ImixMenu::onCategory), i);
        button->setAnchorPoint({0.5f, 0.5f});
        button->setPosition({0.f, 54.f - i * 50.f});
        categoryMenu->addChild(button);
        mCategoryButtons->addObject(button);
    }

    auto footer = text("IMIX 1.0.0", 9.f, MUTED);
    footer->setAnchorPoint({0.f, 0.f});
    footer->setPosition({16.f, 13.f});
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
    auto category = static_cast<CCNode*>(sender)->getTag();
    if (category == mSelectedCategory) return;
    selectCategory(category);
}

void ImixMenu::animateCategoryButtons() {
    if (!mCategoryButtons) return;
    for (unsigned int i = 0; i < mCategoryButtons->count(); ++i) {
        auto button = static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));
        button->stopAllActions();
        button->setColor(i == static_cast<unsigned int>(mSelectedCategory) ? ACCENT : TEXT);
        button->setScale(1.f);
        if (i == static_cast<unsigned int>(mSelectedCategory)) {
            button->runAction(CCSequence::create(
                CCScaleTo::create(0.08f, 1.04f),
                CCEaseSineOut::create(CCScaleTo::create(0.14f, 1.f)),
                nullptr
            ));
        }
    }
}

void ImixMenu::animateContentIn() {
    if (!mContent) return;
    mContent->stopAllActions();
    mContent->setOpacity(0);
    mContent->setPositionX(mContent->getPositionX() + 8.f);
    auto move = CCEaseSineOut::create(CCMoveBy::create(0.16f, {-8.f, 0.f}));
    auto fade = CCFadeIn::create(0.13f);
    mContent->runAction(CCSpawn::create(move, fade, nullptr));
}

void ImixMenu::selectCategory(int category) {
    mSelectedCategory = category;
    if (!mContent) return;
    mContent->removeAllChildrenWithCleanup(true);

    const char* titles[] = {"Smart StartPos", "Player", "Visual"};
    const char* subtitles[] = {
        "Fast level-position workflow",
        "Player-related Imix controls",
        "Interface and visual controls"
    };

    auto contentW = mContent->getContentWidth();
    auto contentH = mContent->getContentHeight();

    auto title = text(titles[category], 20.f, TEXT);
    title->setAnchorPoint({0.f, 1.f});
    title->setPosition({22.f, contentH - 20.f});
    mContent->addChild(title);

    auto subtitle = text(subtitles[category], 10.f, MUTED);
    subtitle->setAnchorPoint({0.f, 1.f});
    subtitle->setPosition({23.f, contentH - 43.f});
    mContent->addChild(subtitle);

    if (category == 0) {
        auto card = CCLayerColor::create({PANEL.r, PANEL.g, PANEL.b, 255});
        card->setContentSize({contentW - 44.f, 82.f});
        card->setPosition({22.f, contentH - 137.f});
        mContent->addChild(card);

        auto label = text("Smart StartPos", 14.f, TEXT);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({15.f, 65.f});
        card->addChild(label);

        auto description = text("Automatic position workflow", 9.f, MUTED);
        description->setAnchorPoint({0.f, 1.f});
        description->setPosition({15.f, 45.f});
        card->addChild(description);

        auto enabled = Mod::get()->getSavedValue<bool>("smart-startpos-enabled", true);
        auto state = text(enabled ? "ENABLED" : "DISABLED", 9.f, enabled ? SUCCESS : MUTED);
        state->setAnchorPoint({1.f, 0.5f});
        state->setPosition({card->getContentWidth() - 15.f, 58.f});
        card->addChild(state);

        auto menu = CCMenu::create();
        menu->setPosition({card->getContentWidth() - 55.f, 20.f});
        card->addChild(menu);

        auto toggle = makeTextButton(enabled ? "Disable" : "Enable", this, menu_selector(ImixMenu::onSmartToggle));
        toggle->setScale(0.82f);
        menu->addChild(toggle);

        auto actionCard = CCLayerColor::create({PANEL.r, PANEL.g, PANEL.b, 255});
        actionCard->setContentSize({contentW - 44.f, 72.f});
        actionCard->setPosition({22.f, contentH - 220.f});
        mContent->addChild(actionCard);

        auto actionTitle = text("Position slot", 12.f, TEXT);
        actionTitle->setAnchorPoint({0.f, 1.f});
        actionTitle->setPosition({15.f, 53.f});
        actionCard->addChild(actionTitle);

        auto actionInfo = text("Capture / clear the active Smart StartPos", 8.f, MUTED);
        actionInfo->setAnchorPoint({0.f, 1.f});
        actionInfo->setPosition({15.f, 35.f});
        actionCard->addChild(actionInfo);

        auto actionMenu = CCMenu::create();
        actionMenu->setPosition({actionCard->getContentWidth() - 55.f, 22.f});
        actionCard->addChild(actionMenu);

        auto capture = makeTextButton("Capture", this, menu_selector(ImixMenu::onSmartAction), 1);
        capture->setScale(0.68f);
        capture->setPositionY(13.f);
        actionMenu->addChild(capture);

        auto clear = makeTextButton("Clear", this, menu_selector(ImixMenu::onSmartAction), 2);
        clear->setScale(0.68f);
        clear->setPositionY(-13.f);
        actionMenu->addChild(clear);

        auto ready = Mod::get()->getSavedValue<bool>("smart-startpos-slot-ready", false);
        auto hint = text(ready ? "Slot ready" : "No slot captured", 8.f, ready ? SUCCESS : MUTED);
        hint->setAnchorPoint({0.f, 0.f});
        hint->setPosition({22.f, 13.f});
        mContent->addChild(hint);
    } else {
        auto card = CCLayerColor::create({PANEL.r, PANEL.g, PANEL.b, 255});
        card->setContentSize({contentW - 44.f, 96.f});
        card->setPosition({22.f, contentH - 151.f});
        mContent->addChild(card);

        auto label = text(category == 1 ? "Player settings" : "Visual settings", 14.f, TEXT);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({15.f, 72.f});
        card->addChild(label);

        auto status = text(
            category == 1 ? "Player controls will be added here" : "Custom Imix visual system is active",
            9.f, MUTED
        );
        status->setAnchorPoint({0.f, 1.f});
        status->setPosition({15.f, 49.f});
        card->addChild(status);

        auto state = text("READY", 9.f, ACCENT);
        state->setAnchorPoint({1.f, 0.5f});
        state->setPosition({card->getContentWidth() - 15.f, 22.f});
        card->addChild(state);
    }

    animateCategoryButtons();
    animateContentIn();
}

void ImixMenu::onSmartToggle(CCObject*) {
    auto enabled = Mod::get()->getSavedValue<bool>("smart-startpos-enabled", true);
    Mod::get()->setSavedValue("smart-startpos-enabled", !enabled);
    selectCategory(0);
}

void ImixMenu::onSmartAction(CCObject* sender) {
    auto tag = static_cast<CCNode*>(sender)->getTag();
    if (tag == 1) Mod::get()->setSavedValue("smart-startpos-slot-ready", true);
    if (tag == 2) Mod::get()->setSavedValue("smart-startpos-slot-ready", false);
    selectCategory(0);
}
