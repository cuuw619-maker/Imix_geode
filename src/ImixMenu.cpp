#include "ImixMenu.hpp"
#include "ImixAI.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <vector>

using namespace geode::prelude;

namespace {
constexpr ccColor3B BG{9, 12, 17}, HEADER{15, 20, 27}, SIDE{12, 16, 22}, PANEL{20, 26, 35};
constexpr ccColor3B TEXT{238, 242, 248}, MUTED{132, 145, 161}, ACCENT{92, 184, 255}, ON{106, 224, 157};
constexpr ccColor3B BUTTON{31, 39, 51}, BUTTON_ON{45, 78, 67};

CCLabelTTF* T(const char* s, float z, ccColor3B c) {
    auto x = CCLabelTTF::create(s, "sans-serif", z);
    x->setColor(c);
    return x;
}

CCLayer* roundedPanel(float w, float h, ccColor4F color, float radius = 10.f) {
    auto layer = CCLayer::create();
    layer->setContentSize({w, h});
    auto d = CCDrawNode::create();
    d->setContentSize({w, h});
    layer->addChild(d);

    std::vector<CCPoint> pts;
    const int steps = 7;
    const float r = std::min(radius, std::min(w, h) * .5f);
    const float pi = 3.14159265359f;
    const float corners[4][2] = {{r, r}, {w-r, r}, {w-r, h-r}, {r, h-r}};
    const float starts[4] = {pi, pi * 1.5f, 0.f, pi * .5f};
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= steps; ++i) {
            const float a = starts[c] + (pi * .5f) * (static_cast<float>(i) / steps);
            pts.push_back({corners[c][0] + std::cos(a) * r, corners[c][1] + std::sin(a) * r});
        }
    }
    d->drawPolygon(pts.data(), static_cast<unsigned>(pts.size()), color, 0.f, color);
    return layer;
}

CCMenuItemSpriteExtra* B(const char* s, CCObject* t, SEL_MenuHandler f, int tag = 0, bool active = false) {
    auto bg = roundedPanel(92.f, 27.f, active ? ccColor4F{BUTTON_ON.r/255.f, BUTTON_ON.g/255.f, BUTTON_ON.b/255.f, 1.f}
                                                : ccColor4F{BUTTON.r/255.f, BUTTON.g/255.f, BUTTON.b/255.f, 1.f}, 7.f);
    auto label = T(s, 9.5f, active ? ON : TEXT);
    label->setPosition({46.f, 13.5f});
    bg->addChild(label);
    auto x = CCMenuItemSpriteExtra::create(bg, t, f);
    x->setTag(tag);
    return x;
}

bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
float SF(const char* k, float d) { return Mod::get()->getSavedValue<float>(k, d); }
void SFSet(const char* k, float v) { Mod::get()->setSavedValue(k, v); }

CCLayer* C(CCLayer* p, float y, float h) {
    auto c = roundedPanel(p->getContentWidth() - 44.f, h,
        {PANEL.r/255.f, PANEL.g/255.f, PANEL.b/255.f, 1.f}, 10.f);
    c->setPosition({22, y});
    p->addChild(c);
    return c;
}

void toggle(CCLayer* p, float y, const char* n, const char* d, const char* k, CCObject* t, SEL_MenuHandler f, int tag) {
    auto c = C(p, y, 43);
    auto l = T(n, 11, TEXT); l->setAnchorPoint({0, 1}); l->setPosition({13, 33}); c->addChild(l);
    auto q = T(d, 7, MUTED); q->setAnchorPoint({0, 1}); q->setPosition({13, 17}); c->addChild(q);
    auto m = CCMenu::create(); m->setPosition({c->getContentWidth()-51, 21});
    m->addChild(B(F(k) ? "ON" : "OFF", t, f, tag, F(k)));
    c->addChild(m);
}
}

bool ImixMenu::init() {
    if (!Popup::init(520, 400)) return false;

    // Hide Geode/GD chrome. Imix draws its own rounded UI and its own close control.
    if (m_bgSprite) m_bgSprite->setVisible(false);
    if (m_closeBtn) m_closeBtn->setVisible(false);

    auto win = CCDirector::sharedDirector()->getWinSize();
    auto s = m_mainLayer->getContentSize();
    setScale(std::min(1.f, std::min(win.width*.90f/s.width, win.height*.82f/s.height)));

    auto r = roundedPanel(s.width, s.height, {BG.r/255.f, BG.g/255.f, BG.b/255.f, 1.f}, 16.f);
    r->setPosition({0, 0});
    m_mainLayer->addChild(r, 100);

    auto h = roundedPanel(s.width, 62, {HEADER.r/255.f, HEADER.g/255.f, HEADER.b/255.f, 1.f}, 14.f);
    h->setPosition({0, s.height-62}); r->addChild(h);
    auto t = T("IMIX", 21, TEXT); t->setAnchorPoint({0,.5f}); t->setPosition({22,40}); h->addChild(t);
    auto sub = T("CUSTOM GAMEPLAY LAB  •  AI TRAINER", 8, MUTED); sub->setAnchorPoint({0,.5f}); sub->setPosition({23,18}); h->addChild(sub);

    // Custom close button, no Geometry Dash texture.
    auto closeLayer = roundedPanel(32, 32, {BUTTON.r/255.f, BUTTON.g/255.f, BUTTON.b/255.f, 1.f}, 10.f);
    auto close = T("×", 23, TEXT); close->setPosition({16, 15}); closeLayer->addChild(close);
    auto closeItem = CCMenuItemSpriteExtra::create(closeLayer, this, menu_selector(ImixMenu::onClose));
    auto closeMenu = CCMenu::create(); closeMenu->setPosition({s.width-24, s.height-31}); closeMenu->addChild(closeItem); r->addChild(closeMenu, 20);

    auto side = roundedPanel(146, s.height-62, {SIDE.r/255.f, SIDE.g/255.f, SIDE.b/255.f, 1.f}, 12.f);
    side->setPosition({0,0}); r->addChild(side);
    mContent = CCLayer::create(); mContent->setContentSize({s.width-146,s.height-62}); mContent->setPosition({146,0}); r->addChild(mContent);

    mCategoryButtons = CCArray::create(); mCategoryButtons->retain();
    auto menu = CCMenu::create(); menu->setPosition({73,s.height-105}); side->addChild(menu);
    const char* cats[] = {"STARTPOS","PLAYER","VISUAL","GAMEPLAY","MOTION","TOOLS","AI TRAINER"};
    for (int i=0;i<7;i++) {
        auto b = B(cats[i], this, menu_selector(ImixMenu::onCategory), i, i == 0);
        b->setPosition({0,92-i*38.f}); menu->addChild(b); mCategoryButtons->addObject(b);
    }
    auto v = T("IMIX CORE",8,MUTED); v->setPosition({16,14}); v->setAnchorPoint({0,0}); side->addChild(v);
    selectCategory(0); return true;
}

ImixMenu* ImixMenu::create() { auto x=new ImixMenu(); if(x->init()){x->autorelease();return x;} delete x; return nullptr; }
void ImixMenu::onCategory(CCObject* s) { int c=static_cast<CCNode*>(s)->getTag(); if(c!=mSelectedCategory) selectCategory(c); }
void ImixMenu::animateCategoryButtons() {
    for(unsigned i=0;mCategoryButtons&&i<mCategoryButtons->count();i++) {
        auto b=static_cast<CCMenuItemSpriteExtra*>(mCategoryButtons->objectAtIndex(i));
        b->stopAllActions(); b->setScale(1.f);
        if(i==(unsigned)mSelectedCategory) b->runAction(CCSequence::create(CCScaleTo::create(.07f,1.07f),CCEaseSineOut::create(CCScaleTo::create(.16f,1.f)),nullptr));
    }
}
void ImixMenu::animateContentIn() { if(!mContent)return; mContent->stopAllActions(); mContent->setPositionX(156); mContent->runAction(CCEaseSineOut::create(CCMoveTo::create(.20f,{146,0}))); }

void ImixMenu::selectCategory(int cat) {
    mSelectedCategory=std::max(0,std::min(cat,6)); mContent->removeAllChildrenWithCleanup(true); float h=mContent->getContentHeight();
    const char* titles[]={"Smart StartPos","Player Lab","Visual Lab","Gameplay","Motion Lab","Utilities","AI Practice Trainer"};
    auto title=T(titles[mSelectedCategory],19,TEXT); title->setAnchorPoint({0,1}); title->setPosition({22,h-18}); mContent->addChild(title);

    if(mSelectedCategory==0){
        auto c=C(mContent,h-132,68); auto l=T("SMART STARTPOS",12,TEXT); l->setPosition({14,49}); c->addChild(l); auto d=T("Capture / restore your live run position",8,MUTED); d->setPosition({14,30}); c->addChild(d);
        auto m=CCMenu::create();m->setPosition({c->getContentWidth()-51,34});m->addChild(B(F("smart-startpos-enabled",true)?"ON":"OFF",this,menu_selector(ImixMenu::onSmartToggle),0,F("smart-startpos-enabled",true)));c->addChild(m);
        auto a=C(mContent,h-209,57);auto i=T(F("startpos-valid")?"SLOT READY":"SLOT EMPTY",9,F("startpos-valid")?ON:MUTED);i->setPosition({14,37});a->addChild(i);auto am=CCMenu::create();am->setPosition({a->getContentWidth()-105,20});a->addChild(am);
        auto cp=B("CAPTURE",this,menu_selector(ImixMenu::onSmartAction),1);cp->setScale(.72f);cp->setPositionX(0);am->addChild(cp);auto go=B("RESTORE",this,menu_selector(ImixMenu::onSmartAction),2);go->setScale(.72f);go->setPositionX(50);am->addChild(go);auto cl=B("CLEAR",this,menu_selector(ImixMenu::onSmartAction),0);cl->setScale(.72f);cl->setPositionX(100);am->addChild(cl);
        auto n=T("Runtime only • level data stays untouched",7,MUTED);n->setPosition({22,19});mContent->addChild(n);
    } else if(mSelectedCategory==1){
        toggle(mContent,h-132,"Ghost Player","Semi-transparent player","ghost-player",this,menu_selector(ImixMenu::onVisualToggle),1);
        toggle(mContent,h-180,"Hide Player","Invisible sprite","hide-player",this,menu_selector(ImixMenu::onVisualToggle),2);
        toggle(mContent,h-228,"Mirror Player","Horizontal flip","mirror-player",this,menu_selector(ImixMenu::onVisualToggle),4);
        toggle(mContent,h-276,"Pulse Scale","Breathing player scale","pulse-scale",this,menu_selector(ImixMenu::onVisualToggle),6);
        auto c=C(mContent,h-324,46);auto l=T("Player Scale",11,TEXT);l->setPosition({13,30});c->addChild(l);
        const float sc=std::clamp(SF("player-scale-factor",1.f),.50f,1.50f);char b[32];std::snprintf(b,sizeof(b),"%.2fx",sc);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-51,23});m->addChild(B(b,this,menu_selector(ImixMenu::onPlayerScale)));c->addChild(m);
        auto info=T("Precision 0.01x  •  50%–150%",7,MUTED);info->setPosition({22,17});mContent->addChild(info);
    } else if(mSelectedCategory==2){
        toggle(mContent,h-132,"Rainbow Player","Animated RGB cycle","rainbow-player",this,menu_selector(ImixMenu::onVisualToggle),3);
        toggle(mContent,h-180,"Color Reactor","Color follows X position","color-reactor",this,menu_selector(ImixMenu::onVisualToggle),7);
        toggle(mContent,h-228,"X-Ray Fade","Dynamic transparency pulse","xray-fade",this,menu_selector(ImixMenu::onVisualToggle),8);
        toggle(mContent,h-276,"Auto Mirror","Flip from movement direction","auto-mirror",this,menu_selector(ImixMenu::onVisualToggle),9);
        toggle(mContent,h-324,"Spin Player","Continuous rotation","spin-player",this,menu_selector(ImixMenu::onVisualToggle),5);
    } else if(mSelectedCategory==3){
        toggle(mContent,h-132,"No Death","Block the normal death callback","no-death",this,menu_selector(ImixMenu::onGameplayToggle),20);
        toggle(mContent,h-180,"Smart Restore","Restore saved position on demand","smart-startpos-enabled",this,menu_selector(ImixMenu::onGameplayToggle),21);
        toggle(mContent,h-228,"Practice Shield","No-death + position utility","practice-shield",this,menu_selector(ImixMenu::onGameplayToggle),22);
        auto c=C(mContent,h-286,72);auto l=T("EXPERIMENTAL",10,TEXT);l->setPosition({14,52});c->addChild(l);auto d=T("Gameplay hooks are isolated from level data",8,MUTED);d->setPosition({14,31});c->addChild(d);auto e=T("Use in practice/testing environments",7,MUTED);e->setPosition({14,16});c->addChild(e);
    } else if(mSelectedCategory==4){
        toggle(mContent,h-132,"Freeze Rotation","Lock player angle at zero","freeze-rotation",this,menu_selector(ImixMenu::onVisualToggle),13);
        toggle(mContent,h-180,"Spin Player","Continuous rotation","spin-player",this,menu_selector(ImixMenu::onVisualToggle),5);
        toggle(mContent,h-228,"Pulse Scale","Smooth sinusoidal scale","pulse-scale",this,menu_selector(ImixMenu::onVisualToggle),6);
        toggle(mContent,h-276,"Auto Mirror","Movement-direction flip","auto-mirror",this,menu_selector(ImixMenu::onVisualToggle),9);
        toggle(mContent,h-324,"X-Ray Fade","Smooth alpha modulation","xray-fade",this,menu_selector(ImixMenu::onVisualToggle),8);
    } else if(mSelectedCategory==5){
        auto c=C(mContent,h-132,78);auto l=T("PANIC RESET",12,TEXT);l->setPosition({14,57});c->addChild(l);auto d=T("Disable every Imix runtime feature",8,MUTED);d->setPosition({14,36});c->addChild(d);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-51,38});m->addChild(B("RESET",this,menu_selector(ImixMenu::onResetFeatures)));c->addChild(m);auto n=T("Settings are stored locally by the mod",7,MUTED);n->setPosition({22,18});mContent->addChild(n);
    } else {
        toggle(mContent,h-132,"AI Auto Pilot","Adaptive search + learned timing","ai-enabled",this,menu_selector(ImixMenu::onAIToggle),1);
        auto c=C(mContent,h-188,100);auto l=T("LEARNING ENGINE",11,TEXT);l->setPosition({14,76});c->addChild(l);
        auto d=T("SEARCH  EARLY  •  CENTER  •  LATE",8,MUTED);d->setPosition({14,56});c->addChild(d);
        auto e=T("Bounded retries: normally 4, max 6",8,MUTED);e->setPosition({14,40});c->addChild(e);
        auto z=T("Stores failure X/Y + chosen hypothesis",7,MUTED);z->setPosition({14,24});c->addChild(z);
        auto m=CCMenu::create();m->setPosition({c->getContentWidth()-51,50});m->addChild(B("CLEAR",this,menu_selector(ImixMenu::onAIReset)));c->addChild(m);
        auto a=C(mContent,h-306,91);auto t1=T("LIVE ANALYSIS",11,TEXT);t1->setPosition({14,67});a->addChild(t1);auto t2=T("Overlay: state / candidate / X-Y / checkpoint",8,MUTED);t2->setPosition({14,46});a->addChild(t2);auto t3=T("Log: [SEARCH] [ACTION] [FAIL] [ROLLBACK]",7,MUTED);t3->setPosition({14,29});a->addChild(t3);auto t4=T("No API key • no network AI",7,MUTED);t4->setPosition({14,15});a->addChild(t4);
        auto note=T("AI Practice mode • learned plans remain local",7,MUTED);note->setAnchorPoint({0,0});note->setPosition({22,18});mContent->addChild(note);
    }
    animateCategoryButtons(); animateContentIn();
}

void ImixMenu::onSmartToggle(CCObject*){Mod::get()->setSavedValue("smart-startpos-enabled",!F("smart-startpos-enabled",true));selectCategory(0);}
void ImixMenu::onSmartAction(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();if(t==1)Mod::get()->setSavedValue("startpos-request-capture",true);else if(t==2)Mod::get()->setSavedValue("startpos-request-teleport",true);else{Mod::get()->setSavedValue("startpos-valid",false);Mod::get()->setSavedValue("startpos-request-capture",false);}selectCategory(0);}
void ImixMenu::onVisualToggle(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();const char* k=t==1?"ghost-player":t==2?"hide-player":t==3?"rainbow-player":t==4?"mirror-player":t==5?"spin-player":t==6?"pulse-scale":t==7?"color-reactor":t==8?"xray-fade":"auto-mirror";Mod::get()->setSavedValue(k,!F(k));selectCategory(mSelectedCategory);}
void ImixMenu::onGameplayToggle(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();const char* k=t==20?"no-death":t==21?"smart-startpos-enabled":"practice-shield";Mod::get()->setSavedValue(k,!F(k));selectCategory(mSelectedCategory);}
void ImixMenu::onPlayerScale(CCObject*){float sc=std::clamp(SF("player-scale-factor",1.f)+.01f,.50f,1.50f);if(sc>=1.50f-.0001f)sc=.50f;SFSet("player-scale-factor",sc);selectCategory(1);}
void ImixMenu::onAIToggle(CCObject*){Mod::get()->setSavedValue("ai-enabled",!F("ai-enabled"));if(F("ai-enabled"))Mod::get()->setSavedValue("practice-shield",true);else ImixAI::reset();selectCategory(6);}
void ImixMenu::onAIReset(CCObject*){ImixAI::reset();selectCategory(6);}
void ImixMenu::onResetFeatures(CCObject*){ImixAI::reset();const char* ks[]={"ghost-player","hide-player","rainbow-player","mirror-player","spin-player","pulse-scale","color-reactor","xray-fade","auto-mirror","no-death","practice-shield","startpos-valid","startpos-request-capture","startpos-request-teleport","smart-startpos-enabled","ai-enabled"};for(auto k:ks)Mod::get()->setSavedValue(k,false);SFSet("player-scale-factor",1.f);selectCategory(0);}
