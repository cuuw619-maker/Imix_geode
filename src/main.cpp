#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include "ImixMenu.hpp"
#include "ImixRuntimeAPI.hpp"
using namespace geode::prelude;

namespace {
class ImixDragHandle : public CCLayerColor {
    CCPoint mLast{};
    bool mDragging = false;
    ImixMenu* mOwner = nullptr;
public:
    static ImixDragHandle* create(ImixMenu* owner, float w, float h) {
        auto x = new ImixDragHandle();
        if (x && x->initWithColor({0,0,0,0})) {
            x->mOwner = owner;
            x->setContentSize({w,h});
            x->autorelease();
            return x;
        }
        delete x;
        return nullptr;
    }
    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!mOwner) return false;
        auto p = convertToNodeSpace(touch->getLocation());
        if (p.x > getContentWidth() - 52.f) return false;
        mLast = touch->getLocation();
        mDragging = true;
        return true;
    }
    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!mDragging || !mOwner) return;
        auto now = touch->getLocation();
        auto delta = now - mLast;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto size = mOwner->getContentSize() * mOwner->getScale();
        const float margin = std::max(8.f, std::min(win.width, win.height) * .018f);
        auto pos = mOwner->getPosition();
        // ImixMenu is top-right anchored: position is the panel's top-right point.
        pos.x = std::clamp(pos.x + delta.x, size.width + margin, win.width - margin);
        pos.y = std::clamp(pos.y + delta.y, size.height + margin, win.height - margin);
        mOwner->setPosition(pos);
        mLast = now;
    }
    void ccTouchEnded(CCTouch*, CCEvent*) override { mDragging = false; }
    void ccTouchCancelled(CCTouch*, CCEvent*) override { mDragging = false; }
    void registerWithTouchDispatcher() override {
        CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, INT_MIN + 20, true);
    }
};

CCLayer* floatingVisual() {
    constexpr float S = 46.f;
    auto root = CCLayer::create();
    root->setContentSize({S,S});
    root->setAnchorPoint({.5f,.5f});
    auto d = CCDrawNode::create();
    d->setContentSize({S,S});
    std::vector<CCPoint> p;
    const float r = 13.f, pi = 3.14159265359f;
    const float cx[4] = {r,S-r,S-r,r}, cy[4] = {r,r,S-r,S-r}, st[4] = {pi,pi*1.5f,0.f,pi*.5f};
    for (int c=0;c<4;c++) for (int i=0;i<=10;i++) {
        float a=st[c]+pi*.5f*(float(i)/10.f);
        p.push_back({cx[c]+std::cos(a)*r,cy[c]+std::sin(a)*r});
    }
    d->drawPolygon(p.data(),(unsigned)p.size(),{.035f,.055f,.085f,.98f},2.f,{.20f,.48f,.72f,.95f});
    root->addChild(d);
    auto label=CCLabelTTF::create("I","sans-serif",18.f);
    label->setColor({235,245,255});
    label->setPosition({S*.5f,S*.5f});
    root->addChild(label,2);
    return root;
}

CCMenuItemSpriteExtra* createFloatingButton(CCObject* target, SEL_MenuHandler cb) {
    auto x=CCMenuItemSpriteExtra::create(floatingVisual(),target,cb);
    x->setContentSize({46,46});
    x->setAnchorPoint({.5f,.5f});
    return x;
}

CCMenu* makeFloatingMenu(CCObject* target, SEL_MenuHandler cb, const char* id, CCPoint pos) {
    auto m=CCMenu::create();
    m->setID(id);
    m->setPosition(pos);
    auto b=createFloatingButton(target,cb);
    b->setPosition({0,0});
    m->addChild(b);
    return m;
}

void animateButton(CCNode* n) {
    if (!n) return;
    n->stopAllActions();
    n->setScale(.94f);
    n->runAction(CCEaseSineOut::create(CCScaleTo::create(.16f,1.f)));
}

void addRuntimeBadge(ImixMenu* popup) {
    if (!popup || popup->getChildByID("imix-runtime-badge")) return;
    auto badge = CCLayerColor::create({20, 28, 39, 235}, 118.f, 22.f);
    badge->setID("imix-runtime-badge");
    badge->setAnchorPoint({0.f, 1.f});
    auto label = CCLabelTTF::create(
        ImixRuntime::version() ? "RUST CORE  •  LOCAL" : "C++ CORE  •  LOCAL",
        "sans-serif", 7.5f
    );
    label->setColor({205,225,242});
    label->setAnchorPoint({0.f, .5f});
    label->setPosition({8.f, 11.f});
    badge->addChild(label);
    auto win = CCDirector::sharedDirector()->getWinSize();
    const auto base = popup->getContentSize();
    const float scale = popup->getScale();
    badge->setPosition({12.f / scale, (base.height - 12.f) / scale});
    popup->addChild(badge, 2000);
}

void openImix(CCNode* button) {
    if (!button) return;
    animateButton(button);

    auto popup = ImixMenu::create();
    if (!popup) return;

    auto win = CCDirector::sharedDirector()->getWinSize();
    const float margin = std::max(12.f, std::min(win.width, win.height) * .025f);
    const auto base = popup->getContentSize();

    // Top-right GD-style placement. The anchor is explicit so the panel can
    // never drift past the right/top edge when its scale changes.
    popup->setAnchorPoint({1.f, 1.f});

    const float availableW = std::max(260.f, win.width - margin * 2.f);
    const float availableH = std::max(220.f, win.height - margin * 2.f);
    float scale = std::min(availableW / base.width, availableH / base.height);
    scale = std::clamp(scale, .58f, 1.f);
    popup->setScale(scale);
    popup->setPosition({win.width - margin, win.height - margin});

    auto drag = ImixDragHandle::create(popup, base.width, 64.f);
    if (drag) {
        drag->setPosition({0.f, base.height - 64.f});
        popup->addChild(drag, 1000);
    }

    addRuntimeBadge(popup);

    popup->stopAllActions();
    popup->setScale(scale * .96f);
    popup->runAction(CCEaseBackOut::create(CCScaleTo::create(.20f, scale)));
}
}

class $modify(ImixMenuLayer,MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false;
        auto w=CCDirector::sharedDirector()->getWinSize();
        auto m=makeFloatingMenu(this,menu_selector(ImixMenuLayer::onImix),"imix-main-menu",{w.width-32.f,32.f});
        addChild(m,10000);
        return true;
    }
    void onImix(CCObject*) {
        if (auto m=getChildByID("imix-main-menu")) openImix(m);
    }
};

class $modify(ImixPauseLayer,PauseLayer) {
public:
    void customSetup() {
        PauseLayer::customSetup();
        auto w=CCDirector::sharedDirector()->getWinSize();
        auto m=makeFloatingMenu(this,menu_selector(ImixPauseLayer::onImixPause),"imix-pause-menu",{w.width-32.f,32.f});
        addChild(m,10000);
    }
    void onImixPause(CCObject*) {
        if (auto m=getChildByID("imix-pause-menu")) openImix(m);
    }
};
