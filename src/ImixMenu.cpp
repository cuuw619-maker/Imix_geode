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

static bool flag(const char* key, bool fallback = false) {
    return Mod::get()->getSavedValue<bool>(key, fallback);
}

static CCNode* card(CCLayer* parent, float x, float y, float w, float h) {
    auto result = CCLayerColor::create({PANEL.r, PANEL.g, PANEL.b, 255});
    result->setContentSize({w, h});
    result->setPosition({x, y});
    parent->addChild(result);
    return result;
}

bool ImixMenu::init() {
    if (!Popup::init(500.f, 315.f, "GJ_square01.png")) return false;
    auto win = CCDirector::sharedDirector()->getWinSize();
    auto popupSize = m_mainLayer->getContentSize();
    auto fitX = (win.width * 0.86f) / popupSize.width;
    auto fitY = (win.height * 0.72f) / popupSize.height;
    this->setScale(std::min(1.0f, std::min(fitX, fitY)));

    auto root = CCLayerColor::create({BG.r, BG.g, BG.b, 255});
    root->setContentSize(popupSize);
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
    auto status = text("ACTIVE", 9.f, SUCCESS);
    status->setAnchorPoint({1.f, 0.5f});
    status->setPosition({popupSize.width - 22.f, 29.f});
    header->addChild(status);

    constexpr float sidebarWidth = 142.f;
    auto sidebar = CCLayerColor::create({SIDEBAR.r, SIDEBAR.g, SIDEBAR.b, 255});
    sidebar->setContentSize({sidebarWidth, popupSize.height - 58.f});
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
    const char* categories[] = {"StartPos", "Player", "Visual", "Tools"};
    for (int i = 0; i < 4; ++i) {
        auto button = makeTextButton(categories[i], this, menu_selector(ImixMenu::onCategory), i);
        button->setPosition({0.f, 72.f - i * 45.f});
        categoryMenu->addChild(button);
        mCategoryButtons->addObject(button);
    }
    auto footer = text("IMIX 1.1", 9.f, MUTED);
    footer->setAnchorPoint({0.f, 0.f});
    footer->setPosition({16.f, 13.f});
    sidebar->addChild(footer);
    selectCategory(0);
    return true;
}

ImixMenu* ImixMenu::create() {
    auto ret = new ImixMenu();
    if (ret->init()) { ret->autorelease(); return ret; }
    delete ret;
    return nullptr;
}

void ImixMenu::onCategory(CCObject* sender) {
    int category = static_cast<CCNode*>(sender)->getTag();
    if (category != mSelectedCategory) selectCategory(category);
}

void ImixMenu::animateCategoryButtons() {
    if (!mCategoryButtons) return;
    for (unsigned int i = 0; i < mCategoryButtons->count(); ++i) {
        auto button = static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));
        button->stopAllActions();
        button->setColor(i == static_cast<unsigned int>(mSelectedCategory) ? ACCENT : TEXT);
        button->setScale(1.f);
        if (i == static_cast<unsigned int>(mSelectedCategory))
            button->runAction(CCSequence::create(CCScaleTo::create(0.08f, 1.04f), CCEaseSineOut::create(CCScaleTo::create(0.14f, 1.f)), nullptr));
    }
}

void ImixMenu::animateContentIn() {
    if (!mContent) return;
    mContent->stopAllActions();
    mContent->setPositionX(mContent->getPositionX() + 8.f);
    mContent->runAction(CCEaseSineOut::create(CCMoveBy::create(0.16f, {-8.f, 0.f})));
}

void ImixMenu::selectCategory(int category) {
    mSelectedCategory = std::max(0, std::min(category, 3));
    if (!mContent) return;
    mContent->removeAllChildrenWithCleanup(true);
    const char* titles[] = {"Smart StartPos", "Player Cheats", "Visual Cheats", "Tools"};
    const char* subs[] = {"Live position capture and teleport", "Player modifiers", "Visual modifiers", "Utilities"};
    auto w = mContent->getContentWidth();
    auto h = mContent->getContentHeight();
    auto title = text(titles[mSelectedCategory], 20.f, TEXT);
    title->setAnchorPoint({0.f, 1.f}); title->setPosition({22.f, h - 20.f}); mContent->addChild(title);
    auto sub = text(subs[mSelectedCategory], 10.f, MUTED);
    sub->setAnchorPoint({0.f, 1.f}); sub->setPosition({23.f, h - 43.f}); mContent->addChild(sub);

    if (mSelectedCategory == 0) {
        auto c = card(mContent, 22.f, h - 137.f, w - 44.f, 82.f);
        auto l = text("Smart StartPos", 14.f, TEXT); l->setPosition({15.f, 62.f}); l->setAnchorPoint({0.f, 1.f}); c->addChild(l);
        auto d = text("Enable position tools in gameplay", 9.f, MUTED); d->setPosition({15.f, 43.f}); d->setAnchorPoint({0.f, 1.f}); c->addChild(d);
        bool enabled = flag("smart-startpos-enabled", true);
        auto b = makeTextButton(enabled ? "ON" : "OFF", this, menu_selector(ImixMenu::onSmartToggle)); b->setScale(.8f); b->setPosition({c->getContentWidth()-38.f, 26.f}); auto cm=CCMenu::create(); cm->setPosition(0,0); cm->addChild(b); c->addChild(cm);
        auto a = card(mContent, 22.f, h - 220.f, w - 44.f, 72.f);
        auto al = text("Position Slot", 12.f, TEXT); al->setPosition({15.f, 53.f}); al->setAnchorPoint({0.f,1.f}); a->addChild(al);
        auto ai = text(flag("startpos-valid") ? "Saved position ready" : "No saved position", 8.f, MUTED); ai->setPosition({15.f,35.f}); ai->setAnchorPoint({0.f,1.f}); a->addChild(ai);
        auto am=CCMenu::create(); am->setPosition({a->getContentWidth()-65.f, 20.f}); a->addChild(am);
        auto cap=makeTextButton("Capture",this,menu_selector(ImixMenu::onSmartAction),1); cap->setScale(.58f); cap->setPositionY(12.f); am->addChild(cap);
        auto go=makeTextButton("Go",this,menu_selector(ImixMenu::onSmartAction),2); go->setScale(.58f); go->setPositionY(-12.f); am->addChild(go);
        auto clr=makeTextButton("Clear",this,menu_selector(ImixMenu::onSmartAction),0); clr->setScale(.58f); clr->setPositionX(-45.f); am->addChild(clr);
    } else if (mSelectedCategory == 1 || mSelectedCategory == 2) {
        const char* names[] = {"Ghost Player", "Hide Player", "Rainbow Player"};
        const char* descs[] = {"50% player opacity", "Hide player sprite", "Animated RGB player"};
        const char* keys[] = {"ghost-player", "hide-player", "rainbow-player"};
        for (int i=0;i<3;++i) {
            auto c=card(mContent,22.f,h-137.f-i*55.f,w-44.f,48.f);
            auto l=text(names[i],12.f,TEXT); l->setAnchorPoint({0.f,1.f}); l->setPosition({13.f,36.f}); c->addChild(l);
            auto d=text(descs[i],8.f,MUTED); d->setAnchorPoint({0.f,1.f}); d->setPosition({13.f,19.f}); c->addChild(d);
            auto menu=CCMenu::create(); menu->setPosition({c->getContentWidth()-38.f,24.f}); c->addChild(menu);
            auto btn=makeTextButton(flag(keys[i])?"ON":"OFF",this,menu_selector(ImixMenu::onVisualToggle),i+1); btn->setScale(.7f); menu->addChild(btn);
        }
        if (mSelectedCategory == 1) {
            auto c=card(mContent,22.f,h-305.f,w-44.f,48.f);
            auto l=text("Player Scale",12.f,TEXT); l->setAnchorPoint({0.f,1.f}); l->setPosition({13.f,36.f}); c->addChild(l);
            int s=Mod::get()->getSavedValue<int>("player-scale",100); char buf[16]; std::snprintf(buf,sizeof(buf),"%d%%",s);
            auto menu=CCMenu::create(); menu->setPosition({c->getContentWidth()-38.f,24.f}); c->addChild(menu); auto btn=makeTextButton(buf,this,menu_selector(ImixMenu::onPlayerScale)); btn->setScale(.7f); menu->addChild(btn);
        }
    } else {
        auto c=card(mContent,22.f,h-137.f,w-44.f,82.f);
        auto l=text("Reset Features",14.f,TEXT); l->setAnchorPoint({0.f,1.f}); l->setPosition({15.f,65.f}); c->addChild(l);
        auto d=text("Disable cheats and clear saved position",9.f,MUTED); d->setAnchorPoint({0.f,1.f}); d->setPosition({15.f,44.f}); c->addChild(d);
        auto menu=CCMenu::create(); menu->setPosition({c->getContentWidth()-42.f,23.f}); c->addChild(menu); auto btn=makeTextButton("RESET",this,menu_selector(ImixMenu::onResetFeatures)); btn->setScale(.68f); menu->addChild(btn);
    }
    animateCategoryButtons(); animateContentIn();
}

void ImixMenu::onSmartToggle(CCObject*) {
    Mod::get()->setSavedValue("smart-startpos-enabled", !flag("smart-startpos-enabled", true));
    selectCategory(0);
}

void ImixMenu::onSmartAction(CCObject* sender) {
    int tag=static_cast<CCNode*>(sender)->getTag();
    if(tag==1) Mod::get()->setSavedValue("startpos-request-capture",true);
    else if(tag==2) Mod::get()->setSavedValue("startpos-request-teleport",true);
    else Mod::get()->setSavedValue("startpos-valid",false);
    selectCategory(0);
}

void ImixMenu::onVisualToggle(CCObject* sender) {
    int tag=static_cast<CCNode*>(sender)->getTag();
    const char* key=tag==1?"ghost-player":tag==2?"hide-player":"rainbow-player";
    Mod::get()->setSavedValue(key,!flag(key));
    selectCategory(mSelectedCategory);
}

void ImixMenu::onPlayerScale(CCObject*) {
    int s=Mod::get()->getSavedValue<int>("player-scale",100)+10;
    if(s>130) s=70;
    Mod::get()->setSavedValue("player-scale",s);
    selectCategory(1);
}

void ImixMenu::onResetFeatures(CCObject*) {
    Mod::get()->setSavedValue("ghost-player",false);
    Mod::get()->setSavedValue("hide-player",false);
    Mod::get()->setSavedValue("rainbow-player",false);
    Mod::get()->setSavedValue("startpos-valid",false);
    Mod::get()->setSavedValue("startpos-request-capture",false);
    Mod::get()->setSavedValue("startpos-request-teleport",false);
    Mod::get()->setSavedValue("player-scale",100);
    selectCategory(3);
}
