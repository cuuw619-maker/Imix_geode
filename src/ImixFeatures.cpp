#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "ImixAI.hpp"
#include <cmath>
#include <algorithm>

using namespace geode::prelude;

namespace {
bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
float clamp01(float v) { return std::max(0.f, std::min(1.f, v)); }
void setHue(PlayerObject* p, float hue) {
    hue -= std::floor(hue);
    float r = std::fabs(hue * 6.f - 3.f) - 1.f;
    float g = 2.f - std::fabs(hue * 6.f - 2.f);
    float b = 2.f - std::fabs(hue * 6.f - 4.f);
    p->setColor({static_cast<GLubyte>(clamp01(r) * 255.f), static_cast<GLubyte>(clamp01(g) * 255.f), static_cast<GLubyte>(clamp01(b) * 255.f)});
}
}

class $modify(ImixPlayLayer, PlayLayer) {
public:
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        // AI must never replace the real death path with noclip/no-death.
        // Practice mode owns death, checkpoint creation and the normal respawn.
        if (ImixAI::enabled()) {
            ImixAI::onDeath(this);
            PlayLayer::destroyPlayer(player, obj);
            return;
        }
        if (F("no-death") || F("practice-shield")) return;
        PlayLayer::destroyPlayer(player, obj);
    }

    void update(float dt) {
        PlayLayer::update(dt);
        auto player = this->m_player1;
        if (!player) return;

        if (ImixAI::enabled()) {
            // Force the actual Geometry Dash practice state. We deliberately do
            // not touch collision/damage flags; the player must really die.
            this->m_isPracticeMode = true;
            Mod::get()->setSavedValue("no-death", false);
            Mod::get()->setSavedValue("practice-shield", false);

            auto scene = CCDirector::sharedDirector()->getRunningScene();
            auto overlay = scene ? scene->getChildByID("imix-ai-overlay") : nullptr;
            if (!overlay && scene) {
                overlay = ImixAI::createOverlay();
                overlay->setID("imix-ai-overlay");
                overlay->setAnchorPoint({0.f, 0.f});
                auto win = CCDirector::sharedDirector()->getWinSize();
                overlay->setPosition({std::max(8.f, win.width * .018f), std::max(8.f, win.height - 118.f)});
                overlay->setScale(std::clamp(std::min(win.width / 800.f, win.height / 450.f), .72f, 1.05f));
                overlay->setOpacity(0);
                scene->addChild(overlay, 100000);
                overlay->runAction(CCSequence::create(
                    CCFadeTo::create(.16f, 255),
                    CCEaseSineOut::create(CCScaleTo::create(.18f, overlay->getScale())),
                    nullptr
                ));
            } else if (overlay) {
                auto win = CCDirector::sharedDirector()->getWinSize();
                overlay->setPosition({std::max(8.f, win.width * .018f), std::max(8.f, win.height - 118.f)});
                overlay->setScale(std::clamp(std::min(win.width / 800.f, win.height / 450.f), .72f, 1.05f));
            }

            ImixAI::update(this, dt);
            // AI owns the player while active. Other experimental player visual
            // hooks are intentionally suspended so they cannot interfere with input.
            return;
        }

        if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
            if (auto overlay = scene->getChildByID("imix-ai-overlay")) {
                overlay->stopAllActions();
                overlay->runAction(CCSequence::create(
                    CCFadeOut::create(.12f),
                    CCRemoveSelf::create(),
                    nullptr
                ));
            }
        }

        if (F("smart-startpos-enabled", true)) {
            if (Mod::get()->getSavedValue<bool>("startpos-request-capture", false)) {
                auto pos = player->getPosition();
                Mod::get()->setSavedValue("startpos-x", pos.x);
                Mod::get()->setSavedValue("startpos-y", pos.y);
                Mod::get()->setSavedValue("startpos-valid", true);
                Mod::get()->setSavedValue("startpos-request-capture", false);
            }
            if (Mod::get()->getSavedValue<bool>("startpos-request-teleport", false)) {
                if (Mod::get()->getSavedValue<bool>("startpos-valid", false)) {
                    auto x = Mod::get()->getSavedValue<float>("startpos-x", player->getPositionX());
                    auto y = Mod::get()->getSavedValue<float>("startpos-y", player->getPositionY());
                    player->setPosition({x, y});
                }
                Mod::get()->setSavedValue("startpos-request-teleport", false);
            }
        }

        static float hue = 0.f;
        static float lastX = 0.f;
        const float x = player->getPositionX();
        const float dx = x - lastX;
        lastX = x;

        const bool hidden = F("hide-player");
        const bool ghost = F("ghost-player");
        int opacity = hidden ? 0 : ghost ? 125 : 255;
        if (F("xray-fade") && !hidden) opacity = static_cast<int>(125.f + 100.f * (0.5f + 0.5f * std::sin(x * 0.035f)));
        player->setOpacity(static_cast<GLubyte>(std::max(0, std::min(255, opacity))));

        if (F("auto-mirror") && std::fabs(dx) > 0.001f) player->setFlipX(dx < 0.f);
        else player->setFlipX(F("mirror-player"));

        if (F("rainbow-player")) { hue += dt * 0.35f; setHue(player, hue); }
        else if (F("color-reactor")) { setHue(player, x * 0.003f + hue * 0.25f); hue += dt * 0.12f; }
        else player->setColor({255,255,255});

        const float baseScale = std::clamp(Mod::get()->getSavedValue<float>("player-scale-factor", 1.f), .50f, 1.50f);
        float scale = baseScale;
        if (F("pulse-scale")) scale *= 1.f + 0.10f * std::sin(x * 0.045f + hue * 6.28318f);
        player->setScale(scale);

        if (F("spin-player")) player->setRotation(player->getRotation() + dt * 240.f);
        else if (F("freeze-rotation")) player->setRotation(0.f);
    }
};
