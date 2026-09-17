#include "ImixMenu.hpp"
#include <algorithm>
#include <cstdio>
using namespace geode::prelude;

namespace {
constexpr ccColor3B BG{11,14,19}, HEADER{17,21,28}, SIDE{14,18,24}, PANEL{23,29,38};
constexpr ccColor3B TEXT{238,242,248}, MUTED{133,145,160}, ACCENT{92,184,255}, ON{106,224,157};
CCLabelTTF* T(const char* s,float z,ccColor3B c){auto x=CCLabelTTF::create(s,"sans-serif",z);x->setColor(c);return x;}
CCMenuItemLabel* B(const char* s,CCObject* t,SEL_MenuHandler f,int tag=0){auto x=CCMenuItemLabel::create(T(s,12,TEXT),t,f);x->setTag(tag);return x;}
bool F(const char* k,bool d=false){return Mod::get()->getSavedValue<bool>(k,d);}
CCLayerColor* C(CCLayer* p,float y,float h){auto c=CCLayerColor::create({PANEL.r,PANEL.g,PANEL.b,255});c->setContentSize({p->getContentWidth()-44,h});c->setPosition({22,y});p->addChild(c);return c;}
void toggle(CCLayer* p,float y,const char* n,const char* d,const char* k,CCObject* t,SEL_MenuHandler f,int tag){auto c=C(p,y,42);auto l=T(n,11,TEXT);l->setAnchorPoint({0,1});l->setPosition({13,32});c->addChild(l);auto q=T(d,7,MUTED);q->setAnchorPoint({0,1});q->setPosition({13,16});c->addChild(q);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-34,21});auto b=B(F(k)?"ON":"OFF",t,f,tag);b->setScale(.67f);m->addChild(b);c->addChild(m);}
}

bool ImixMenu::init(){
    if(!Popup::init(520,400,"GJ_square01.png")) return false;
    auto win=CCDirector::sharedDirector()->getWinSize(),s=m_mainLayer->getContentSize();
    setScale(std::min(1.f,std::min(win.width*.90f/s.width,win.height*.82f/s.height)));
    auto r=CCLayerColor::create({BG.r,BG.g,BG.b,255});r->setContentSize(s);m_mainLayer->addChild(r,100);
    auto h=CCLayerColor::create({HEADER.r,HEADER.g,HEADER.b,255});h->setContentSize({s.width,62});h->setPosition({0,s.height-62});r->addChild(h);
    auto t=T("IMIX",21,TEXT);t->setAnchorPoint({0,.5f});t->setPosition({22,40});h->addChild(t);
    auto sub=T("CUSTOM GAMEPLAY LAB  •  v2",8,MUTED);sub->setAnchorPoint({0,.5f});sub->setPosition({23,18});h->addChild(sub);
    auto dot=CCLayerColor::create({ON.r,ON.g,ON.b,255});dot->setContentSize({7,7});dot->setPosition({s.width-24,42});h->addChild(dot);
    auto side=CCLayerColor::create({SIDE.r,SIDE.g,SIDE.b,255});side->setContentSize({146,s.height-62});r->addChild(side);
    mContent=CCLayer::create();mContent->setContentSize({s.width-146,s.height-62});mContent->setPosition({146,0});r->addChild(mContent);
    mCategoryButtons=CCArray::create();mCategoryButtons->retain();
    auto menu=CCMenu::create();menu->setPosition({73,s.height-110});side->addChild(menu);
    const char* cats[]={"STARTPOS","PLAYER","VISUAL","GAMEPLAY","MOTION","TOOLS"};
    for(int i=0;i<6;i++){auto b=B(cats[i],this,menu_selector(ImixMenu::onCategory),i);b->setPosition({0,92-i*43.f});menu->addChild(b);mCategoryButtons->addObject(b);}
    auto v=T("IMIX CORE",8,MUTED);v->setPosition({16,14});v->setAnchorPoint({0,0});side->addChild(v);
    selectCategory(0);return true;
}

ImixMenu* ImixMenu::create(){auto x=new ImixMenu();if(x->init()){x->autorelease();return x;}delete x;return nullptr;}
void ImixMenu::onCategory(CCObject* s){int c=static_cast<CCNode*>(s)->getTag();if(c!=mSelectedCategory)selectCategory(c);}
void ImixMenu::animateCategoryButtons(){for(unsigned i=0;mCategoryButtons&&i<mCategoryButtons->count();i++){auto b=static_cast<CCMenuItemLabel*>(mCategoryButtons->objectAtIndex(i));b->stopAllActions();b->setColor(i==(unsigned)mSelectedCategory?ACCENT:TEXT);b->setScale(1);if(i==(unsigned)mSelectedCategory)b->runAction(CCSequence::create(CCScaleTo::create(.07f,1.08f),CCEaseSineOut::create(CCScaleTo::create(.16f,1)),nullptr));}}
void ImixMenu::animateContentIn(){if(!mContent)return;mContent->stopAllActions();mContent->setPositionX(156);mContent->runAction(CCEaseSineOut::create(CCMoveTo::create(.20f,{146,0})));}

void ImixMenu::selectCategory(int cat){
    mSelectedCategory=std::max(0,std::min(cat,5));mContent->removeAllChildrenWithCleanup(true);float h=mContent->getContentHeight();
    const char* titles[]={"Smart StartPos","Player Lab","Visual Lab","Gameplay","Motion Lab","Utilities"};
    auto title=T(titles[mSelectedCategory],19,TEXT);title->setAnchorPoint({0,1});title->setPosition({22,h-18});mContent->addChild(title);
    if(mSelectedCategory==0){
        auto c=C(mContent,h-132,68);auto l=T("SMART STARTPOS",12,TEXT);l->setPosition({14,49});c->addChild(l);auto d=T("Capture / restore your live run position",8,MUTED);d->setPosition({14,30});c->addChild(d);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-37,34});m->addChild(B(F("smart-startpos-enabled",true)?"ON":"OFF",this,menu_selector(ImixMenu::onSmartToggle)));c->addChild(m);
        auto a=C(mContent,h-209,57);auto i=T(F("startpos-valid")?"SLOT READY":"SLOT EMPTY",9,F("startpos-valid")?ON:MUTED);i->setPosition({14,37});a->addChild(i);auto am=CCMenu::create();am->setPosition({a->getContentWidth()-55,20});a->addChild(am);auto cp=B("CAPTURE",this,menu_selector(ImixMenu::onSmartAction),1);cp->setScale(.55);cp->setPositionY(10);am->addChild(cp);auto go=B("RESTORE",this,menu_selector(ImixMenu::onSmartAction),2);go->setScale(.55);go->setPositionY(-10);am->addChild(go);auto cl=B("CLEAR",this,menu_selector(ImixMenu::onSmartAction),0);cl->setScale(.55);cl->setPositionX(-43);am->addChild(cl);auto n=T("Runtime only • level data stays untouched",7,MUTED);n->setPosition({22,19});mContent->addChild(n);
    }else if(mSelectedCategory==1){
        toggle(mContent,h-132,"Ghost Player","Semi-transparent player","ghost-player",this,menu_selector(ImixMenu::onVisualToggle),1);
        toggle(mContent,h-180,"Hide Player","Invisible sprite","hide-player",this,menu_selector(ImixMenu::onVisualToggle),2);
        toggle(mContent,h-228,"Mirror Player","Horizontal flip","mirror-player",this,menu_selector(ImixMenu::onVisualToggle),4);
        toggle(mContent,h-276,"Pulse Scale","Breathing player scale","pulse-scale",this,menu_selector(ImixMenu::onVisualToggle),6);
        auto c=C(mContent,h-324,42);auto l=T("Player Scale",11,TEXT);l->setPosition({13,27});c->addChild(l);int sc=Mod::get()->getSavedValue<int>("player-scale",100);char b[20];std::snprintf(b,sizeof(b),"%d%%",sc);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-40,21});m->addChild(B(b,this,menu_selector(ImixMenu::onPlayerScale)));c->addChild(m);
    }else if(mSelectedCategory==2){
        toggle(mContent,h-132,"Rainbow Player","Animated RGB cycle","rainbow-player",this,menu_selector(ImixMenu::onVisualToggle),3);
        toggle(mContent,h-180,"Color Reactor","Color follows X position","color-reactor",this,menu_selector(ImixMenu::onVisualToggle),7);
        toggle(mContent,h-228,"X-Ray Fade","Dynamic transparency pulse","xray-fade",this,menu_selector(ImixMenu::onVisualToggle),8);
        toggle(mContent,h-276,"Auto Mirror","Flip from movement direction","auto-mirror",this,menu_selector(ImixMenu::onVisualToggle),9);
        toggle(mContent,h-324,"Spin Player","Continuous rotation","spin-player",this,menu_selector(ImixMenu::onVisualToggle),5);
    }else if(mSelectedCategory==3){
        toggle(mContent,h-132,"No Death","Block the normal death callback","no-death",this,menu_selector(ImixMenu::onGameplayToggle),20);
        toggle(mContent,h-180,"Smart Restore","Restore saved position on demand","smart-startpos-enabled",this,menu_selector(ImixMenu::onGameplayToggle),21);
        toggle(mContent,h-228,"Practice Shield","No-death + position utility","practice-shield",this,menu_selector(ImixMenu::onGameplayToggle),22);
        auto c=C(mContent,h-286,72);auto l=T("EXPERIMENTAL",10,TEXT);l->setPosition({14,52});c->addChild(l);auto d=T("Gameplay hooks are isolated from level data",8,MUTED);d->setPosition({14,31});c->addChild(d);auto e=T("Use in practice/testing environments",7,MUTED);e->setPosition({14,16});c->addChild(e);
    }else if(mSelectedCategory==4){
        toggle(mContent,h-132,"Freeze Rotation","Lock player angle at zero","freeze-rotation",this,menu_selector(ImixMenu::onVisualToggle),13);
        toggle(mContent,h-180,"Spin Player","Continuous rotation","spin-player",this,menu_selector(ImixMenu::onVisualToggle),5);
        toggle(mContent,h-228,"Pulse Scale","Smooth sinusoidal scale","pulse-scale",this,menu_selector(ImixMenu::onVisualToggle),6);
        toggle(mContent,h-276,"Auto Mirror","Movement-direction flip","auto-mirror",this,menu_selector(ImixMenu::onVisualToggle),9);
        toggle(mContent,h-324,"X-Ray Fade","Smooth alpha modulation","xray-fade",this,menu_selector(ImixMenu::onVisualToggle),8);
    }else{
        auto c=C(mContent,h-132,78);auto l=T("PANIC RESET",12,TEXT);l->setPosition({14,57});c->addChild(l);auto d=T("Disable every Imix runtime feature",8,MUTED);d->setPosition({14,36});c->addChild(d);auto m=CCMenu::create();m->setPosition({c->getContentWidth()-43,38});m->addChild(B("RESET",this,menu_selector(ImixMenu::onResetFeatures)));c->addChild(m);auto n=T("Settings are stored locally by the mod",7,MUTED);n->setPosition({22,18});mContent->addChild(n);
    }
    animateCategoryButtons();animateContentIn();
}

void ImixMenu::onSmartToggle(CCObject*){Mod::get()->setSavedValue("smart-startpos-enabled",!F("smart-startpos-enabled",true));selectCategory(0);}
void ImixMenu::onSmartAction(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();if(t==1)Mod::get()->setSavedValue("startpos-request-capture",true);else if(t==2)Mod::get()->setSavedValue("startpos-request-teleport",true);else{Mod::get()->setSavedValue("startpos-valid",false);Mod::get()->setSavedValue("startpos-request-capture",false);}selectCategory(0);}
void ImixMenu::onVisualToggle(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();const char* k=t==1?"ghost-player":t==2?"hide-player":t==3?"rainbow-player":t==4?"mirror-player":t==5?"spin-player":t==6?"pulse-scale":t==7?"color-reactor":t==8?"xray-fade":"auto-mirror";Mod::get()->setSavedValue(k,!F(k));selectCategory(mSelectedCategory);}
void ImixMenu::onGameplayToggle(CCObject* s){int t=static_cast<CCNode*>(s)->getTag();const char* k=t==20?"no-death":t==21?"smart-startpos-enabled":"practice-shield";Mod::get()->setSavedValue(k,!F(k));selectCategory(mSelectedCategory);}
void ImixMenu::onPlayerScale(CCObject*){int sc=Mod::get()->getSavedValue<int>("player-scale",100)+10;if(sc>140)sc=60;Mod::get()->setSavedValue("player-scale",sc);selectCategory(1);}
void ImixMenu::onResetFeatures(CCObject*){const char* ks[]={"ghost-player","hide-player","rainbow-player","mirror-player","spin-player","pulse-scale","color-reactor","xray-fade","auto-mirror","no-death","practice-shield","startpos-valid","startpos-request-capture","startpos-request-teleport","smart-startpos-enabled"};for(auto k:ks)Mod::get()->setSavedValue(k,false);Mod::get()->setSavedValue("player-scale",100);selectCategory(0);}
