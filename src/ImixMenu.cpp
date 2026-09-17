#include "ImixMenu.hpp"
#include "ImixAI.hpp"
#include "ImixRuntimeAPI.hpp"
#include <algorithm>
#include <cstdio>

using namespace geode::prelude;
using namespace ImixMenuModel;

namespace {
constexpr float W = 460.f;
constexpr float H = 330.f;
constexpr float SIDEBAR_W = 154.f;
constexpr float CONTENT_X = 166.f;
constexpr ccColor3B TEXT{255, 255, 255};
constexpr ccColor3B MUTED{185, 185, 185};
constexpr ccColor3B GOOD{130, 255, 170};

bool F(const char* key, bool fallback = false) {
    return Mod::get()->getSavedValue<bool>(key, fallback);
}
float SF(const char* key, float fallback) {
    return Mod::get()->getSavedValue<float>(key, fallback);
}
void SFSet(const char* key, float value) {
    Mod::get()->setSavedValue(key, value);
}

CCLabelBMFont* text(const char* value, float scale = .52f, ccColor3B color = TEXT) {
    auto out = CCLabelBMFont::create(value, "bigFont.fnt");
    out->setScale(scale);
    out->setColor(color);
    return out;
}

CCScale9Sprite* panel(float width, float height, GLubyte opacity = 65) {
    auto p = CCScale9Sprite::create("GJ_square01.png");
    if (!p) return nullptr;
    p->setContentSize({width, height});
    p->setOpacity(opacity);
    return p;
}

CCMenuItemSpriteExtra* button(const char* caption, CCObject* target, SEL_MenuHandler cb,
                              int tag, bool selected = false, float width = 104.f) {
    auto bg = CCScale9Sprite::create(selected ? "GJ_button_02.png" : "GJ_button_01.png");
    if (!bg) return nullptr;
    bg->setContentSize({width, 34.f});
    auto label = text(caption, .49f);
    label->setPosition({width * .5f, 17.f});
    bg->addChild(label);
    auto item = CCMenuItemSpriteExtra::create(bg, target, cb);
    item->setTag(tag);
    return item;
}

void fitPopup(ImixMenu* popup) {
    auto win = CCDirector::sharedDirector()->getWinSize();
    const float margin = std::max(10.f, std::min(win.width, win.height) * .035f);
    const float scale = std::min(1.f, std::min((win.width - margin * 2.f) / W,
                                                (win.height - margin * 2.f) / H));
    popup->setScale(std::max(.58f, scale));
}

void addToggleButton(CCLayer* row, CCObject* target, SEL_MenuHandler cb, int tag,
                     const char* key, float width = 72.f) {
    auto menu = CCMenu::create();
    menu->setPosition({row->getContentWidth() - width * .5f - 10.f, row->getContentHeight() * .5f});
    auto b = button(F(key) ? "ON" : "OFF", target, cb, tag, F(key), width);
    if (b) menu->addChild(b);
    row->addChild(menu);
}

void addDescription(CCLayer* row, const Item& item) {
    auto title = text(item.title, .52f);
    title->setAnchorPoint({0.f, .5f});
    title->setPosition({12.f, row->getContentHeight() - 15.f});
    row->addChild(title);

    auto desc = text(item.description, .33f, MUTED);
    desc->setAnchorPoint({0.f, .5f});
    desc->setPosition({12.f, 13.f});
    row->addChild(desc);
}
}

bool ImixMenu::init() {
    if (!Popup::init(W, H)) return false;
    fitPopup(this);

    if (m_closeBtn) {
        m_closeBtn->setPosition({W - 18.f, H - 18.f});
        m_closeBtn->setScale(.78f);
    }

    auto title = text("IMIX", .82f);
    title->setPosition({W * .5f, H - 26.f});
    m_mainLayer->addChild(title, 5);

    auto subtitle = text("RUNTIME / GAMEPLAY / VISUAL / AI", .32f, MUTED);
    subtitle->setPosition({W * .5f, H - 45.f});
    m_mainLayer->addChild(subtitle, 5);

    auto divider = CCLayerColor::create({255, 255, 255, 42}, W - 32.f, 1.f);
    divider->setPosition({16.f, H - 58.f});
    m_mainLayer->addChild(divider, 5);

    auto sidebar = panel(SIDEBAR_W, H - 76.f, 50);
    if (sidebar) {
        sidebar->setPosition({14.f + SIDEBAR_W * .5f, 12.f + (H - 76.f) * .5f});
        m_mainLayer->addChild(sidebar, 1);
    }

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    auto categoriesMenu = CCMenu::create();
    categoriesMenu->setPosition({14.f + SIDEBAR_W * .5f, H - 84.f});
    m_mainLayer->addChild(categoriesMenu, 4);

    const auto& cats = categories();
    for (int i = 0; i < static_cast<int>(cats.size()); ++i) {
        auto b = button(cats[i].title, this, menu_selector(ImixMenu::onCategory),
                        i, i == 0, SIDEBAR_W - 18.f);
        if (!b) continue;
        b->setPosition({0.f, -i * 32.f});
        categoriesMenu->addChild(b);
        mCategoryButtons->addObject(b);
    }

    auto footer = text("IMIX  •  C ABI  •  LOCAL", .29f, MUTED);
    footer->setAnchorPoint({0.f, .5f});
    footer->setPosition({22.f, 18.f});
    m_mainLayer->addChild(footer, 5);

    mContent = CCLayer::create();
    mContent->setContentSize({W - CONTENT_X - 12.f, H - 76.f});
    mContent->setPosition({CONTENT_X, 12.f});
    m_mainLayer->addChild(mContent, 3);

    selectCategory(0);
    return true;
}

ImixMenu* ImixMenu::create() {
    auto out = new ImixMenu();
    if (out && out->init()) {
        out->autorelease();
        return out;
    }
    delete out;
    return nullptr;
}

void ImixMenu::onCategory(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    selectCategory(node->getTag());
}

void ImixMenu::animateCategoryButtons() {
    for (unsigned i = 0; mCategoryButtons && i < mCategoryButtons->count(); ++i) {
        auto b = static_cast<CCMenuItemSpriteExtra*>(mCategoryButtons->objectAtIndex(i));
        b->stopActionByTag(7100);
        if (i == static_cast<unsigned>(mSelectedCategory)) {
            auto seq = CCSequence::create(
                CCScaleTo::create(.06f, 1.035f),
                CCEaseSineOut::create(CCScaleTo::create(.12f, 1.f)), nullptr);
            seq->setTag(7100);
            b->runAction(seq);
        } else {
            b->runAction(CCEaseSineOut::create(CCScaleTo::create(.08f, 1.f)));
        }
    }
}

void ImixMenu::animateContentIn(int direction) {
    if (!mContent) return;
    // Never snap to a new start position. This keeps rapid category taps smooth.
    const float target = CONTENT_X;
    const float from = target + (direction >= 0 ? 12.f : -12.f);
    auto current = mContent->getPositionX();
    if (std::fabs(current - target) < 0.5f) current = from;
    mContent->stopActionByTag(7200);
    auto move = CCEaseSineOut::create(CCMoveTo::create(.17f, {target, 12.f}));
    move->setTag(7200);
    mContent->runAction(move);
}

void ImixMenu::selectCategory(int category) {
    const int next = clampCategory(category);
    mPreviousCategory = mSelectedCategory;
    mSelectedCategory = next;
    mContent->removeAllChildrenWithCleanup(true);
    renderCategory();
    animateCategoryButtons();
    animateContentIn(mSelectedCategory >= mPreviousCategory ? 1 : -1);
}

void ImixMenu::renderItem(const Item& item, float y) {
    auto row = panel(mContent->getContentWidth(), item.type == ItemType::Info ? 46.f : 48.f, 52);
    if (!row) return;
    row->setAnchorPoint({0.f, 0.f});
    row->setPosition({0.f, y});
    mContent->addChild(row);
    addDescription(row, item);

    if (item.type == ItemType::Toggle) {
        SEL_MenuHandler cb = menu_selector(ImixMenu::onVisualToggle);
        const auto* cat = findCategory(mSelectedCategory);
        if (cat && std::string(cat->id) == "startpos") cb = menu_selector(ImixMenu::onSmartToggle);
        if (cat && std::string(cat->id) == "gameplay") cb = menu_selector(ImixMenu::onGameplayToggle);
        if (cat && std::string(cat->id) == "ai") cb = menu_selector(ImixMenu::onAIToggle);
        addToggleButton(row, this, cb, item.action, item.key);
        return;
    }

    if (item.type == ItemType::Value) {
        char value[32];
        std::snprintf(value, sizeof(value), "%.2fx", SF(item.key, 1.f));
        auto menu = CCMenu::create();
        menu->setPosition({row->getContentWidth() - 50.f, row->getContentHeight() * .5f});
        if (auto b = button(value, this, menu_selector(ImixMenu::onPlayerScale), item.action, false, 74.f))
            menu->addChild(b);
        row->addChild(menu);
        return;
    }

    if (item.type == ItemType::Action) {
        const auto* cat = findCategory(mSelectedCategory);
        SEL_MenuHandler cb = menu_selector(ImixMenu::onSmartAction);
        if (cat && std::string(cat->id) == "tools") cb = menu_selector(ImixMenu::onResetFeatures);
        if (cat && std::string(cat->id) == "ai") cb = menu_selector(ImixMenu::onAIReset);
        const char* caption = item.title;
        auto menu = CCMenu::create();
        menu->setPosition({row->getContentWidth() - 52.f, row->getContentHeight() * .5f});
        if (auto b = button(caption, this, cb, item.action, false, 82.f)) menu->addChild(b);
        row->addChild(menu);
        return;
    }

    if (item.type == ItemType::Info) {
        const auto* cat = findCategory(mSelectedCategory);
        const bool runtime = cat && std::string(cat->id) == "tools" && std::string(item.id) == "runtime";
        const bool api = cat && std::string(cat->id) == "tools" && std::string(item.id) == "abi";
        const char* value = runtime ? ImixRuntime::backendName() :
                            api ? (ImixRuntime::selfTest() ? "ABI 3 • READY" : "ABI ERROR") :
                            "LOCAL";
        auto v = text(value, .38f, runtime || api ? GOOD : MUTED);
        v->setAnchorPoint({1.f, .5f});
        v->setPosition({row->getContentWidth() - 12.f, row->getContentHeight() * .5f});
        row->addChild(v);
    }
}

void ImixMenu::renderCategory() {
    const auto* cat = findCategory(mSelectedCategory);
    if (!cat || !mContent) return;

    auto heading = text(cat->title, .66f);
    heading->setAnchorPoint({0.f, 1.f});
    heading->setPosition({4.f, mContent->getContentHeight() - 2.f});
    mContent->addChild(heading);

    auto subtitle = text(cat->subtitle, .30f, MUTED);
    subtitle->setAnchorPoint({0.f, 1.f});
    subtitle->setPosition({5.f, mContent->getContentHeight() - 22.f});
    mContent->addChild(subtitle);

    float y = mContent->getContentHeight() - 76.f;
    for (const auto& item : cat->items) {
        const float height = item.type == ItemType::Info ? 46.f : 48.f;
        renderItem(item, y);
        y -= height + 7.f;
    }

    if (std::string(cat->id) == "startpos") {
        auto note = text(F("startpos-valid") ? "SLOT READY" : "SLOT EMPTY", .32f,
                         F("startpos-valid") ? GOOD : MUTED);
        note->setAnchorPoint({0.f, .5f});
        note->setPosition({4.f, 10.f});
        mContent->addChild(note);
    } else if (std::string(cat->id) == "ai") {
        auto stats = ImixRuntime::stats();
        char buf[64];
        std::snprintf(buf, sizeof(buf), "ATTEMPTS %u  •  FAILURES %u", stats.attempts, stats.failures);
        auto telemetry = text(buf, .30f, MUTED);
        telemetry->setAnchorPoint({0.f, .5f});
        telemetry->setPosition({4.f, 10.f});
        mContent->addChild(telemetry);
    }
}

void ImixMenu::onSmartToggle(CCObject*) {
    Mod::get()->setSavedValue("smart-startpos-enabled", !F("smart-startpos-enabled", true));
    selectCategory(mSelectedCategory);
}

void ImixMenu::onSmartAction(CCObject* sender) {
    const int action = static_cast<CCNode*>(sender)->getTag();
    if (action == 1) Mod::get()->setSavedValue("startpos-request-capture", true);
    else if (action == 2) Mod::get()->setSavedValue("startpos-request-teleport", true);
    else {
        Mod::get()->setSavedValue("startpos-valid", false);
        Mod::get()->setSavedValue("startpos-request-capture", false);
        Mod::get()->setSavedValue("startpos-request-teleport", false);
    }
    selectCategory(mSelectedCategory);
}

void ImixMenu::onVisualToggle(CCObject* sender) {
    const int tag = static_cast<CCNode*>(sender)->getTag();
    const char* key = tag == 1 ? "ghost-player" : tag == 2 ? "hide-player" :
                      tag == 3 ? "rainbow-player" : tag == 4 ? "mirror-player" :
                      tag == 5 ? "spin-player" : tag == 6 ? "pulse-scale" :
                      tag == 7 ? "color-reactor" : tag == 8 ? "xray-fade" :
                      tag == 9 ? "auto-mirror" : "freeze-rotation";
    Mod::get()->setSavedValue(key, !F(key));
    selectCategory(mSelectedCategory);
}

void ImixMenu::onGameplayToggle(CCObject* sender) {
    const int tag = static_cast<CCNode*>(sender)->getTag();
    const char* key = tag == 20 ? "no-death" : tag == 21 ? "smart-startpos-enabled" : "practice-shield";
    Mod::get()->setSavedValue(key, !F(key));
    selectCategory(mSelectedCategory);
}

void ImixMenu::onPlayerScale(CCObject*) {
    float scale = SF("player-scale-factor", 1.f) + .01f;
    if (scale > 1.50f) scale = .50f;
    SFSet("player-scale-factor", scale);
    selectCategory(mSelectedCategory);
}

void ImixMenu::onAIToggle(CCObject*) {
    const bool enabled = !F("ai-enabled");
    Mod::get()->setSavedValue("ai-enabled", enabled);
    if (enabled) Mod::get()->setSavedValue("practice-shield", true);
    else ImixAI::reset();
    selectCategory(mSelectedCategory);
}

void ImixMenu::onAIReset(CCObject*) {
    ImixAI::reset();
    ImixRuntime::reset();
    selectCategory(mSelectedCategory);
}

void ImixMenu::onResetFeatures(CCObject*) {
    ImixAI::reset();
    ImixRuntime::reset();
    const char* keys[] = {
        "ghost-player", "hide-player", "rainbow-player", "mirror-player", "spin-player",
        "pulse-scale", "color-reactor", "xray-fade", "auto-mirror", "freeze-rotation",
        "no-death", "practice-shield", "startpos-valid", "startpos-request-capture",
        "startpos-request-teleport", "smart-startpos-enabled", "ai-enabled"
    };
    for (auto key : keys) Mod::get()->setSavedValue(key, false);
    SFSet("player-scale-factor", 1.f);
    selectCategory(0);
}
