#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>
#include <algorithm>

using namespace geode::prelude;

namespace {
bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
}

class $modify(ImixPlayLayer, PlayLayer) {
public:
    void update(float dt) {
        PlayLayer::update(dt);
        auto player = this->m_player1;
        if (!player) return;

        // Smart StartPos is consumed from the live gameplay thread, so pressing
        // Capture/Restore from the pause popup affects the actual player.
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

        const bool hidden = F("hide-player");
        const bool ghost = F("ghost-player");
        const bool rainbow = F("rainbow-player");
        player->setOpacity(hidden ? 0 : ghost ? 125 : 255);

        if (F("mirror-player")) player->setFlipX(true);
        else player->setFlipX(false);

        if (rainbow) {
            static float hue = 0.f;
            hue += dt * 0.35f;
            while (hue > 1.f) hue -= 1.f;
            float r = std::fabs(hue * 6.f - 3.f) - 1.f;
            float g = 2.f - std::fabs(hue * 6.f - 2.f);
            float b = 2.f - std::fabs(hue * 6.f - 4.f);
            r = std::max(0.f, std::min(1.f, r));
            g = std::max(0.f, std::min(1.f, g));
            b = std::max(0.f, std::min(1.f, b));
            player->setColor({
                static_cast<GLubyte>(r * 255.f),
                static_cast<GLubyte>(g * 255.f),
                static_cast<GLubyte>(b * 255.f)
            });
        } else {
            player->setColor({255, 255, 255});
        }

        const int scale = Mod::get()->getSavedValue<int>("player-scale", 100);
        player->setScale(scale / 100.f);

        if (F("spin-player")) {
            player->setRotation(player->getRotation() + dt * 240.f);
        } else if (F("freeze-rotation")) {
            player->setRotation(0.f);
        }
    }
};
