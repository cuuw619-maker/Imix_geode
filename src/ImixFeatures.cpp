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
        if (ImixAI::enabled()) {
            ImixAI::onDeath(this);
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
            auto overlay = this->getChildByID("imix-ai-overlay");
            if (!overlay) {
                overlay = ImixAI::createOverlay();
                overlay->setID("imix-ai-overlay");
                overlay->setAnchorPoint({0.f, 0.f});
                overlay->setPosition({12.f, this->getContentHeight() - 124.f});
                overlay->setScale(.82f);
                // createOverlay() returns the CCLayer interface but the concrete root
                // is a CCLayerColor so the HUD can use the RGBA fade safely.
                if (auto rgba = typeinfo_cast<CCLayerColor*>(overlay)) rgba->setOpacity(0);
                this->addChild(overlay, 10000);
                overlay->runAction(CCSequence::create(
                    CCFadeTo::create(.18f, 255),
                    CCEaseSineOut::create(CCScaleTo::create(.16f, .88f)),
                    nullptr
                ));
            } else {
                overlay->setPosition({12.f, this->getContentHeight() - 124.f});
            }
            ImixAI::update(this, dt);
        } else if (auto overlay = this->getChildByID("imix-ai-overlay")) {
            overlay->stopAllActions();
            overlay->runAction(CCFadeOut::create(.12f));
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
        if (F("xray-fade") && !hidden) {
            opacity = static_cast<int>(125.f + 100.f * (0.5f + 0.5f * std::sin(x * 0.035f)));
        }
        player->setOpacity(static_cast<GLubyte>(std::max(0, std::min(255, opacity))));

        if (F("auto-mirror") && std::fabs(dx) > 0.001f) player->setFlipX(dx < 0.f);
        else player->setFlipX(F("mirror-player"));

        if (F("rainbow-player")) {
            hue += dt * 0.35f;
            setHue(player, hue);
        } else if (F("color-reactor")) {
            setHue(player, x * 0.003f + hue * 0.25f);
            hue += dt * 0.12f;
        } else {
            player->setColor({255,255,255});
        }

        const float baseScale = std::clamp(Mod::get()->getSavedValue<float>("player-scale-factor", 1.f), .50f, 1.50f);
        float scale = baseScale;
        if (F("pulse-scale")) scale *= 1.f + 0.10f * std::sin(x * 0.045f + hue * 6.28318f);
        player->setScale(scale);

        if (F("spin-player")) player->setRotation(player->getRotation() + dt * 240.f);
        else if (F("freeze-rotation")) player->setRotation(0.f);
    }
};
