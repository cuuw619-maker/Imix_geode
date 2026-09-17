#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include "ImixMenu.hpp"

using namespace geode::prelude;

namespace {

class ImixDragHandle : public CCLayerColor {
    CCPoint mLast{};
    bool mDragging = false;
    ImixMenu* mOwner = nullptr;

public:
    static ImixDragHandle* create(ImixMenu* owner, float w, float h) {
        auto x = new ImixDragHandle();
        if (x && x->initWithColor({0, 0, 0, 0})) {
            x->mOwner = owner;
            x->setContentSize({w, h});
            x->setTouchEnabled(true);
            x->autorelease();
            return x;
        }
        delete x;
        return nullptr;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!mOwner || !touch) return false;
        auto p = this->convertToNodeSpace(touch->getLocation());
        if (p.x < 0.f || p.y < 0.f || p.x > getContentWidth() || p.y > getContentHeight())
            return false;
        // Leave the close-button area to the actual close button.
        if (p.x > getContentWidth() - 52.f)
            return false;
        mLast = touch->getLocation();
        mDragging = true;
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!mDragging || !mOwner || !touch) return;

        const auto now = touch->getLocation();
        const auto delta = now - mLast;
        auto pos = mOwner->getPosition() + delta;

        auto win = CCDirector::sharedDirector()->getWinSize();
        const auto size = mOwner->getContentSize();
        const float scale = std::max(.01f, mOwner->getScale());
        const float halfW = size.width * scale * .5f;
        const float halfH = size.height * scale * .5f;
        const float margin = 8.f;

        pos.x = std::clamp(pos.x, halfW + margin, win.width - halfW - margin);
        pos.y = std::clamp(pos.y, halfH + margin, win.height - halfH - margin);
        mOwner->setPosition(pos);
        mLast = now;
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override { mDragging = false; }
    void ccTouchCancelled(CCTouch*, CCEvent*) override { mDragging = false; }

    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(
            this, INT_MIN + 5, true
        );
    }
};

CCLayer* floatingVisual() {
    constexpr float S = 48.f;
    auto root = CCLayer::create();
    root->setContentSize({S, S});
    root->setAnchorPoint({.5f, .5f});

    auto d = CCDrawNode::create();
    d->setContentSize({S, S});
    std::vector<CCPoint> p;
    const float r = 14.f;
    const float pi = 3.14159265359f;
    const float cx[4] = {r, S-r, S-r, r};
    const float cy[4] = {r, r, S-r, S-r};
    const float st[4] = {pi, pi*1.5f, 0.f, pi*.5f};
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= 10; i++) {
            const float a = st[c] + pi*.5f * (float(i) / 10.f);
            p.push_back({cx[c] + std::cos(a) * r, cy[c] + std::sin(a) * r});
        }
    }
    d->drawPolygon(
        p.data(), static_cast<unsigned>(p.size()),
        {.035f, .055f, .085f, .98f}, 2.f,
        {.20f, .48f, .72f, .95f}
    );
    root->addChild(d);

    auto label = CCLabelTTF::create("I", "sans-serif", 18.f);
    label->setColor({235, 245, 255});
    label->setPosition({S*.5f, S*.5f});
    root->addChild(label, 2);
    return root;
}

CCMenuItemSpriteExtra* createFloatingButton(CCObject* target, SEL_MenuHandler cb) {
    auto x = CCMenuItemSpriteExtra::create(floatingVisual(), target, cb);
    x->setContentSize({48, 48});
    x->setAnchorPoint({.5f, .5f});
    return x;
}

CCMenu* makeFloatingMenu(CCObject* target, SEL_MenuHandler cb, const char* id, CCPoint pos) {
    auto m = CCMenu::create();
    m->setID(id);
    m->setPosition(pos);
    auto b = createFloatingButton(target, cb);
    b->setPosition({0, 0});
    m->addChild(b);
    return m;
}

void animateButton(CCNode* n) {
    if (!n) return;
    n->stopAllActions();
    n->setScale(.94f);
    n->runAction(CCEaseSineOut::create(CCScaleTo::create(.16f, 1.f)));
}

void openImix(CCNode* button) {
    if (!button) return;
    animateButton(button);

    auto popup = ImixMenu::create();
    if (!popup) return;

    popup->show();

    // ImixMenu already calculates a responsive scale from the real viewport.
    // Do not apply another arbitrary 0.72/0.74 multiplier here: that was making
    // the Android popup unnecessarily tiny and also made its visual center wrong.
    auto win = CCDirector::sharedDirector()->getWinSize();
    const float targetScale = popup->getScale();
    popup->setPosition({win.width * .5f, win.height * .5f});
    popup->setScale(targetScale * .94f);

    auto size = popup->getContentSize();
    auto drag = ImixDragHandle::create(popup, size.width, 62.f);
    if (drag) {
        // Popup is centered; its header uses the same local coordinate system.
        drag->setAnchorPoint({0.f, 0.f});
        drag->setPosition({0.f, size.height - 62.f});
        popup->addChild(drag, 10000);
    }

    popup->runAction(
        CCEaseBackOut::create(CCScaleTo::create(.24f, targetScale))
    );
}

}

class $modify(ImixMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        auto w = CCDirector::sharedDirector()->getWinSize();
        auto m = makeFloatingMenu(
            this,
            menu_selector(ImixMenuLayer::onImix),
            "imix-main-menu",
            {w.width - 34.f, 34.f}
        );
        addChild(m, 10000);
        return true;
    }

    void onImix(CCObject*) {
        if (auto m = getChildByID("imix-main-menu"))
            openImix(m);
    }
};

class $modify(ImixPauseLayer, PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        auto w = CCDirector::sharedDirector()->getWinSize();
        auto m = makeFloatingMenu(
            this,
            menu_selector(ImixPauseLayer::onImixPause),
            "imix-pause-menu",
            {w.width - 34.f, 34.f}
        );
        addChild(m, 10000);
    }

    void onImixPause(CCObject*) {
        if (auto m = getChildByID("imix-pause-menu"))
            openImix(m);
    }
};
