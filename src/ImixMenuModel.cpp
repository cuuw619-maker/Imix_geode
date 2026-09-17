#include "ImixMenuModel.hpp"
#include <algorithm>

namespace ImixMenuModel {

const std::vector<Category>& categories() {
    static const std::vector<Category> value = {
        {"startpos", "SMART STARTPOS", "LIVE POSITION", {
            {"smart", "SMART STARTPOS", "Capture and restore a runtime position", ItemType::Toggle, "smart-startpos-enabled", 0},
            {"capture", "CAPTURE", "Save the current player position", ItemType::Action, nullptr, 1},
            {"restore", "RESTORE", "Teleport to the saved position", ItemType::Action, nullptr, 2},
            {"clear", "CLEAR", "Remove the saved runtime position", ItemType::Action, nullptr, 0},
        }},
        {"player", "PLAYER", "PLAYER CONTROL", {
            {"ghost", "GHOST PLAYER", "Semi-transparent player", ItemType::Toggle, "ghost-player", 1},
            {"hide", "HIDE PLAYER", "Hide the player sprite", ItemType::Toggle, "hide-player", 2},
            {"mirror", "MIRROR PLAYER", "Horizontal flip", ItemType::Toggle, "mirror-player", 4},
            {"pulse", "PULSE SCALE", "Smooth player scale animation", ItemType::Toggle, "pulse-scale", 6},
            {"scale", "PLAYER SCALE", "Change player scale", ItemType::Value, "player-scale-factor", 0},
        }},
        {"visual", "VISUAL", "RENDERING", {
            {"rainbow", "RAINBOW PLAYER", "Animated color cycle", ItemType::Toggle, "rainbow-player", 3},
            {"reactor", "COLOR REACTOR", "Color follows player X", ItemType::Toggle, "color-reactor", 7},
            {"xray", "X-RAY FADE", "Dynamic transparency", ItemType::Toggle, "xray-fade", 8},
            {"automirror", "AUTO MIRROR", "Flip from movement direction", ItemType::Toggle, "auto-mirror", 9},
            {"spin", "SPIN PLAYER", "Continuous rotation", ItemType::Toggle, "spin-player", 5},
        }},
        {"gameplay", "GAMEPLAY", "RUN CONTROL", {
            {"nodeath", "NO DEATH", "Block the normal death callback", ItemType::Toggle, "no-death", 20},
            {"restore", "SMART RESTORE", "Restore the saved position", ItemType::Toggle, "smart-startpos-enabled", 21},
            {"shield", "PRACTICE SHIELD", "Position utility for testing", ItemType::Toggle, "practice-shield", 22},
        }},
        {"motion", "MOTION", "PLAYER MOTION", {
            {"freeze", "FREEZE ROTATION", "Lock player angle", ItemType::Toggle, "freeze-rotation", 13},
            {"spin", "SPIN PLAYER", "Continuous rotation", ItemType::Toggle, "spin-player", 5},
            {"pulse", "PULSE SCALE", "Smooth sinusoidal scale", ItemType::Toggle, "pulse-scale", 6},
            {"mirror", "AUTO MIRROR", "Movement-direction flip", ItemType::Toggle, "auto-mirror", 9},
        }},
        {"tools", "TOOLS", "RUNTIME", {
            {"reset", "RESET IMIX", "Disable features and clear runtime state", ItemType::Action, nullptr, 100},
            {"runtime", "RUNTIME", "Current execution backend", ItemType::Info, nullptr, 0},
            {"abi", "API", "C ABI shared by C++ / Rust / Kotlin", ItemType::Info, nullptr, 0},
        }},
        {"ai", "AI TRAINER", "ADAPTIVE PRACTICE", {
            {"pilot", "AI AUTO PILOT", "Adaptive timing search + failure memory", ItemType::Toggle, "ai-enabled", 1},
            {"learning", "LEARNING ENGINE", "EARLY / CENTER / LATE with adaptive retries", ItemType::Info, nullptr, 0},
            {"clear", "CLEAR AI MEMORY", "Reset planner state and local failure history", ItemType::Action, nullptr, 101},
        }},
    };
    return value;
}

const Category* findCategory(int index) {
    const auto& all = categories();
    if (index < 0 || index >= static_cast<int>(all.size())) return nullptr;
    return &all[static_cast<std::size_t>(index)];
}

int clampCategory(int index) {
    const auto& all = categories();
    if (all.empty()) return 0;
    return std::clamp(index, 0, static_cast<int>(all.size()) - 1);
}

} // namespace ImixMenuModel
