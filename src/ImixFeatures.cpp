#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>

using namespace geode::prelude;

class $modify(ImixPlayLayer, PlayLayer) {
public:
    void update(float dt) {
        PlayLayer::update(dt);
        auto player = this->m_player1;
        if (!player) return;

        if (Mod::get()->getSavedValue<bool>("smart-startpos-enabled", true)) {
            // The current menu's Capture button sets this flag. Save the live position once.
            if (Mod::get()->getSavedValue<bool>("smart-startpos-captured", false) &&
                !Mod::get()->getSavedValue<bool>("startpos-valid", false)) {
                auto pos = player->getPosition();
                Mod::get()->setSavedValue("startpos-x", pos.x);
                Mod::get()->setSavedValue("startpos-y", pos.y);
                Mod::get()->setSavedValue("startpos-valid", true);
            }
            // Clear in the current menu removes the saved slot.
            if (!Mod::get()->getSavedValue<bool>("smart-startpos-captured", false) &&
                Mod::get()->getSavedValue<bool>("startpos-valid", false)) {
                Mod::get()->setSavedValue("startpos-valid", false);
            }
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

        bool hidden = Mod::get()->getSavedValue<bool>("hide-player", false);
        bool ghost = Mod::get()->getSavedValue<bool>("ghost-player", false);
        bool rainbow = Mod::get()->getSavedValue<bool>("rainbow-player", false);
        player->setOpacity(hidden ? 0 : ghost ? 125 : 255);

        if (rainbow) {
            static float hue = 0.f;
            hue += dt * 0.35f;
            if (hue > 1.f) hue -= 1.f;
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

        int scale = Mod::get()->getSavedValue<int>("player-scale", 100);
        player->setScale(scale / 100.f);
    }
};
