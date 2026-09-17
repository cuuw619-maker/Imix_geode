#include "ImixMenu.hpp"
#include "ImixAI.hpp"
#include "ImixRuntimeAPI.hpp"
#include <algorithm>
#include <cstdio>

using namespace geode::prelude;

namespace {
constexpr float W = 460.f;
constexpr float H = 330.f;
constexpr ccColor3B TEXT{255, 255, 255};
constexpr ccColor3B MUTED{190, 190, 190};

bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
float SF(const char* k, float d) { return Mod::get()->getSavedValue<float>(k, d); }
void SFSet(const char* k, float v) { Mod::get()->setSavedValue(k, v); }

CCLabelBMFont* label(const char* text, float scale = 0.55f, ccColor3B color = TEXT) {
    auto x = CCLabelBMFont::create(text, "bigFont.fnt");
    x->setScale(scale);
    x->setColor(color);
    return x;
}

CCMenuItemSpriteExtra* gdButton(const char* text, CCObject* target, SEL_MenuHandler cb,
                                int tag = 0, bool selected = false, float width = 112.f) {
    auto bg = CCScale9Sprite::create(selected ? "GJ_button_02.png" : "GJ_button_01.png");
    if (!bg) return nullptr;
    bg->setContentSize({width, 34.f});
    auto l = label(text, 0.52f);
    l->setPosition({width * .5f, 17.f});
    bg->addChild(l);
    auto item = CCMenuItemSpriteExtra::create(bg, target, cb);
    item->setTag(tag);
    return item;
}

CCLayerColor* section(CCLayer* parent, float y, float height) {
    auto p = CCLayerColor::create({0, 0, 0, 70}, W - 185.f, height);
    p->setPosition({175.f, y});
    parent->addChild(p);
    return p;
}

void toggleRow(CCLayer* parent, float y, const char* name, const char* desc,
               const char* key, CCObject* target, SEL_MenuHandler cb, int tag) {
    auto row = section(parent, y, 44.f);
    auto n = label(name, .56f); n->setAnchorPoint({0.f, .5f}); n->setPosition({12.f, 29.f}); row->addChild(n);
    auto d = label(desc, .36f, MUTED); d->setAnchorPoint({0.f, .5f}); d->setPosition({12.f, 13.f}); row->addChild(d);
    auto menu = CCMenu::create(); menu->setPosition({row->getContentWidth() - 52.f, 22.f});
    menu->addChild(gdButton(F(key) ? "ON" : "OFF", target, cb, tag, F(key), 78.f));
    row->addChild(menu);
}

void fitPopup(ImixMenu* popup) {
    auto win = CCDirector::sharedDirector()->getWinSize();
    const float margin = std::max(10.f, std::min(win.width, win.height) * .04f);
    const float scale = std::min(1.f, std::min((win.width - margin * 2.f) / W,
                                                (win.height - margin * 2.f) / H));
    popup->setScale(std::max(.58f, scale));
}
}

bool ImixMenu::init() {
    if (!Popup::init(W, H)) return false;
    fitPopup(this);

    // Keep the real Geometry Dash/Geode popup chrome. No custom floating window.
    if (m_closeBtn) {
        m_closeBtn->setPosition({W - 18.f, H - 18.f});
        m_closeBtn->setScale(.8f);
    }

    auto title = label("IMIX", .82f);
    title->setPosition({W * .5f, H - 27.f});
    m_mainLayer->addChild(title, 5);

    auto sub = label("GAMEPLAY / VISUAL / SMART STARTPOS / AI", .34f, MUTED);
    sub->setPosition({W * .5f, H - 46.f});
    m_mainLayer->addChild(sub, 5);

    auto line = CCLayerColor::create({255,255,255,45}, W - 32.f, 1.f);
    line->setPosition({16.f, H - 58.f});
    m_mainLayer->addChild(line);

    mCategoryButtons = CCArray::create();
    mCategoryButtons->retain();

    auto side = CCLayerColor::create({0,0,0,45}, 154.f, H - 70.f);
    side->setPosition({14.f, 12.f});
    m_mainLayer->addChild(side);

    auto cats = CCMenu::create();
    cats->setPosition({77.f, H - 86.f});
    m_mainLayer->addChild(cats, 4);

    const char* names[] = {"STARTPOS", "PLAYER", "VISUAL", "GAMEPLAY", "MOTION", "TOOLS", "AI TRAINER"};
    for (int i = 0; i < 7; ++i) {
        auto b = gdButton(names[i], this, menu_selector(ImixMenu::onCategory), i, i == 0, 132.f);
        if (!b) continue;
        b->setPosition({0.f, -i * 32.f});
        cats->addChild(b);
        mCategoryButtons->addObject(b);
    }

    auto footer = label("IMIX  •  LOCAL RUNTIME", .32f, MUTED);
    footer->setAnchorPoint({0.f, .5f});
    footer->setPosition({24.f, 18.f});
    m_mainLayer->addChild(footer);

    mContent = CCLayer::create();
    mContent->setContentSize({W - 174.f, H - 76.f});
    mContent->setPosition({166.f, 12.f});
    m_mainLayer->addChild(mContent, 3);

    selectCategory(0);
    return true;
}

ImixMenu* ImixMenu::create() {
    auto x = new ImixMenu();
    if (x && x->init()) { x->autorelease(); return x; }
    delete x;
    return nullptr;
}

void ImixMenu::onCategory(CCObject* sender) {
    auto node = static_cast<CCNode*>(sender);
    selectCategory(node->getTag());
}

void ImixMenu::animateCategoryButtons() {
    for (unsigned i = 0; mCategoryButtons && i < mCategoryButtons->count(); ++i) {
        auto b = static_cast<CCMenuItemSpriteExtra*>(mCategoryButtons->objectAtIndex(i));
        b->stopAllActions();
        b->setScale(1.f);
        if (i == static_cast<unsigned>(mSelectedCategory)) {
            b->runAction(CCSequence::create(
                CCScaleTo::create(.06f, 1.05f),
                CCEaseSineOut::create(CCScaleTo::create(.12f, 1.f)), nullptr));
        }
    }
}

void ImixMenu::animateContentIn() {
    if (!mContent) return;
    mContent->stopAllActions();
    mContent->setPositionX(172.f);
    mContent->runAction(CCEaseSineOut::create(CCMoveTo::create(.16f, {166.f, 12.f})));
}

void ImixMenu::selectCategory(int cat) {
    mSelectedCategory = std::max(0, std::min(cat, 6));
    mContent->removeAllChildrenWithCleanup(true);
    const float h = mContent->getContentHeight();

    const char* titles[] = {
        "SMART STARTPOS", "PLAYER", "VISUAL", "GAMEPLAY", "MOTION", "TOOLS", "AI PRACTICE"
    };
    auto title = label(titles[mSelectedCategory], .68f);
    title->setAnchorPoint({0.f, 1.f});
    title->setPosition({10.f, h - 4.f});
    mContent->addChild(title);

    if (mSelectedCategory == 0) {
        toggleRow(mContent, h - 72.f, "SMART STARTPOS", "Capture / restore a live run position", "smart-startpos-enabled", this, menu_selector(ImixMenu::onSmartToggle), 0);
        auto row = section(mContent, h - 139.f, 54.f);
        auto state = label(F("startpos-valid") ? "SLOT READY" : "SLOT EMPTY", .48f, F("startpos-valid") ? ccColor3B{120,255,160} : MUTED);
        state->setAnchorPoint({0.f,.5f}); state->setPosition({12.f,27.f}); row->addChild(state);
        auto menu = CCMenu::create(); menu->setPosition({row->getContentWidth() - 105.f, 27.f});
        auto cap = gdButton("CAPTURE", this, menu_selector(ImixMenu::onSmartAction), 1, false, 74.f);
        auto rst = gdButton("RESTORE", this, menu_selector(ImixMenu::onSmartAction), 2, false, 74.f);
        auto clr = gdButton("CLEAR", this, menu_selector(ImixMenu::onSmartAction), 0, false, 74.f);
        cap->setPosition({-76.f,0}); rst->setPosition({0,0}); clr->setPosition({76.f,0});
        menu->addChild(cap); menu->addChild(rst); menu->addChild(clr); row->addChild(menu);
        auto note = label("Runtime only • level data is not modified", .35f, MUTED);
        note->setAnchorPoint({0.f,.5f}); note->setPosition({10.f,18.f}); mContent->addChild(note);
    }
    else if (mSelectedCategory == 1) {
        toggleRow(mContent, h-72.f, "GHOST PLAYER", "Semi-transparent player", "ghost-player", this, menu_selector(ImixMenu::onVisualToggle), 1);
        toggleRow(mContent, h-120.f, "HIDE PLAYER", "Hide the player sprite", "hide-player", this, menu_selector(ImixMenu::onVisualToggle), 2);
        toggleRow(mContent, h-168.f, "MIRROR PLAYER", "Horizontal flip", "mirror-player", this, menu_selector(ImixMenu::onVisualToggle), 4);
        toggleRow(mContent, h-216.f, "PULSE SCALE", "Smooth player scale animation", "pulse-scale", this, menu_selector(ImixMenu::onVisualToggle), 6);
        auto row = section(mContent, h-272.f, 48.f);
        auto l = label("PLAYER SCALE", .48f); l->setAnchorPoint({0,.5f}); l->setPosition({12,28}); row->addChild(l);
        char buf[32]; std::snprintf(buf, sizeof(buf), "%.2fx", SF("player-scale-factor", 1.f));
        auto menu = CCMenu::create(); menu->setPosition({row->getContentWidth()-45.f,24}); menu->addChild(gdButton(buf,this,menu_selector(ImixMenu::onPlayerScale),0,false,74.f)); row->addChild(menu);
    }
    else if (mSelectedCategory == 2) {
        toggleRow(mContent, h-72.f, "RAINBOW PLAYER", "Animated color cycle", "rainbow-player", this, menu_selector(ImixMenu::onVisualToggle), 3);
        toggleRow(mContent, h-120.f, "COLOR REACTOR", "Color follows player X", "color-reactor", this, menu_selector(ImixMenu::onVisualToggle), 7);
        toggleRow(mContent, h-168.f, "X-RAY FADE", "Dynamic transparency", "xray-fade", this, menu_selector(ImixMenu::onVisualToggle), 8);
        toggleRow(mContent, h-216.f, "AUTO MIRROR", "Flip from movement direction", "auto-mirror", this, menu_selector(ImixMenu::onVisualToggle), 9);
        toggleRow(mContent, h-264.f, "SPIN PLAYER", "Continuous rotation", "spin-player", this, menu_selector(ImixMenu::onVisualToggle), 5);
    }
    else if (mSelectedCategory == 3) {
        toggleRow(mContent, h-72.f, "NO DEATH", "Block the normal death callback", "no-death", this, menu_selector(ImixMenu::onGameplayToggle), 20);
        toggleRow(mContent, h-120.f, "SMART RESTORE", "Restore the saved position", "smart-startpos-enabled", this, menu_selector(ImixMenu::onGameplayToggle), 21);
        toggleRow(mContent, h-168.f, "PRACTICE SHIELD", "Position utility for testing", "practice-shield", this, menu_selector(ImixMenu::onGameplayToggle), 22);
    }
    else if (mSelectedCategory == 4) {
        toggleRow(mContent, h-72.f, "FREEZE ROTATION", "Lock player angle", "freeze-rotation", this, menu_selector(ImixMenu::onVisualToggle), 13);
        toggleRow(mContent, h-120.f, "SPIN PLAYER", "Continuous rotation", "spin-player", this, menu_selector(ImixMenu::onVisualToggle), 5);
        toggleRow(mContent, h-168.f, "PULSE SCALE", "Smooth sinusoidal scale", "pulse-scale", this, menu_selector(ImixMenu::onVisualToggle), 6);
        toggleRow(mContent, h-216.f, "AUTO MIRROR", "Movement-direction flip", "auto-mirror", this, menu_selector(ImixMenu::onVisualToggle), 9);
    }
    else if (mSelectedCategory == 5) {
        auto row = section(mContent, h-90.f, 64.f);
        auto l = label("RESET IMIX", .60f); l->setAnchorPoint({0,.5f}); l->setPosition({12,40}); row->addChild(l);
        auto d = label("Disable runtime features and clear AI state", .34f, MUTED); d->setAnchorPoint({0,.5f}); d->setPosition({12,21}); row->addChild(d);
        auto menu = CCMenu::create(); menu->setPosition({row->getContentWidth()-46.f,32}); menu->addChild(gdButton("RESET",this,menu_selector(ImixMenu::onResetFeatures),0,false,82.f)); row->addChild(menu);
        auto api = label(ImixRuntime::backendName(), .38f, MUTED); api->setAnchorPoint({0,.5f}); api->setPosition({10,20}); mContent->addChild(api);
    }
    else {
        toggleRow(mContent, h-72.f, "AI AUTO PILOT", "Adaptive timing search + failure memory", "ai-enabled", this, menu_selector(ImixMenu::onAIToggle), 1);
        auto row = section(mContent, h-155.f, 66.f);
        auto l = label("LEARNING ENGINE", .52f); l->setAnchorPoint({0,.5f}); l->setPosition({12,47}); row->addChild(l);
        auto d = label("EARLY / CENTER / LATE • normally 4, max 6", .35f, MUTED); d->setAnchorPoint({0,.5f}); d->setPosition({12,29}); row->addChild(d);
        auto e = label("Failure X/Y and timing hypothesis stay local", .32f, MUTED); e->setAnchorPoint({0,.5f}); e->setPosition({12,14}); row->addChild(e);
        auto menu = CCMenu::create(); menu->setPosition({row->getContentWidth()-45.f,33}); menu->addChild(gdButton("CLEAR",this,menu_selector(ImixMenu::onAIReset),0,false,78.f)); row->addChild(menu);
        auto rt = label("RUNTIME: LOCAL", .34f, MUTED); rt->setAnchorPoint({0,.5f}); rt->setPosition({10,18}); mContent->addChild(rt);
    }

    animateCategoryButtons();
    animateContentIn();
}

void ImixMenu::onSmartToggle(CCObject*) {
    Mod::get()->setSavedValue("smart-startpos-enabled", !F("smart-startpos-enabled", true));
    selectCategory(0);
}
void ImixMenu::onSmartAction(CCObject* sender) {
    int t = static_cast<CCNode*>(sender)->getTag();
    if (t == 1) Mod::get()->setSavedValue("startpos-request-capture", true);
    else if (t == 2) Mod::get()->setSavedValue("startpos-request-teleport", true);
    else { Mod::get()->setSavedValue("startpos-valid", false); Mod::get()->setSavedValue("startpos-request-capture", false); }
    selectCategory(0);
}
void ImixMenu::onVisualToggle(CCObject* sender) {
    int t = static_cast<CCNode*>(sender)->getTag();
    const char* k = t==1 ? "ghost-player" : t==2 ? "hide-player" : t==3 ? "rainbow-player" : t==4 ? "mirror-player" : t==5 ? "spin-player" : t==6 ? "pulse-scale" : t==7 ? "color-reactor" : t==8 ? "xray-fade" : t==9 ? "auto-mirror" : "freeze-rotation";
    Mod::get()->setSavedValue(k, !F(k));
    selectCategory(mSelectedCategory);
}
void ImixMenu::onGameplayToggle(CCObject* sender) {
    int t = static_cast<CCNode*>(sender)->getTag();
    const char* k = t==20 ? "no-death" : t==21 ? "smart-startpos-enabled" : "practice-shield";
    Mod::get()->setSavedValue(k, !F(k));
    selectCategory(mSelectedCategory);
}
void ImixMenu::onPlayerScale(CCObject*) {
    float sc = std::clamp(SF("player-scale-factor",1.f)+.01f,.50f,1.50f);
    if (sc >= 1.50f-.0001f) sc=.50f;
    SFSet("player-scale-factor",sc);
    selectCategory(1);
}
void ImixMenu::onAIToggle(CCObject*) {
    Mod::get()->setSavedValue("ai-enabled", !F("ai-enabled"));
    if (F("ai-enabled")) Mod::get()->setSavedValue("practice-shield", true);
    else ImixAI::reset();
    selectCategory(6);
}
void ImixMenu::onAIReset(CCObject*) { ImixAI::reset(); selectCategory(6); }
void ImixMenu::onResetFeatures(CCObject*) {
    ImixAI::reset();
    const char* ks[] = {"ghost-player","hide-player","rainbow-player","mirror-player","spin-player","pulse-scale","color-reactor","xray-fade","auto-mirror","freeze-rotation","no-death","practice-shield","startpos-valid","startpos-request-capture","startpos-request-teleport","smart-startpos-enabled","ai-enabled"};
    for (auto k : ks) Mod::get()->setSavedValue(k, false);
    SFSet("player-scale-factor", 1.f);
    selectCategory(0);
}
