#include "ImixAI.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace geode::prelude;

namespace ImixAI {
namespace {
    bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
    float V(const char* k, float d) { return Mod::get()->getSavedValue<float>(k, d); }
    void SV(const char* k, float v) { Mod::get()->setSavedValue(k, v); }
    int I(const char* k, int d) { return Mod::get()->getSavedValue<int>(k, d); }
    void SI(const char* k, int v) { Mod::get()->setSavedValue(k, v); }

    struct State {
        PlayLayer* layer = nullptr;
        CCLabelTTF* overlay = nullptr;
        float nextDecisionX = 70.f;
        float jumpSpacing = 86.f;
        float jumpWindow = 24.f;
        float checkpointX = 0.f;
        float checkpointY = 0.f;
        float lastX = 0.f;
        float lastY = 0.f;
        float time = 0.f;
        float actionTimer = 0.f;
        float lastActionX = -100000.f;
        float failureX = -1.f;
        int failures = 0;
        int attempts = 0;
        int planRevision = 0;
        bool holding = false;
        bool initialized = false;
    } s;

    void text(const char* phase, float x, float y, int rev) {
        if (!s.overlay) return;
        char buf[256];
        std::snprintf(buf, sizeof(buf), "IMIX AI  |  %s\nX %.0f  Y %.0f  |  plan %d  |  fails %d\nnext %.0f  spacing %.1f  window %.1f",
            phase, x, y, rev, s.failures, s.nextDecisionX, s.jumpSpacing, s.jumpWindow);
        s.overlay->setString(buf);
    }

    void tap(PlayLayer* layer) {
        if (!layer) return;
        // Geometry Dash's PlayLayer button bridge lets the bot use the same input path as a player.
        layer->handleButton(true, 1, true);
        s.holding = true;
        s.actionTimer = 0.f;
        s.lastActionX = layer->m_player1 ? layer->m_player1->getPositionX() : s.lastActionX;
    }

    void release(PlayLayer* layer) {
        if (!layer || !s.holding) return;
        layer->handleButton(false, 1, true);
        s.holding = false;
    }
}

bool enabled() {
    return F("ai-enabled", false);
}

CCLabelTTF* createOverlay() {
    auto l = CCLabelTTF::create("IMIX AI  |  initializing", "sans-serif", 9.f);
    l->setColor({230, 235, 245});
    l->setAnchorPoint({0.f, 1.f});
    l->setPosition({10.f, 10.f});
    return l;
}

void reset() {
    if (s.layer) release(s.layer);
    s = {};
    SI("ai-failures", 0);
    SI("ai-attempts", 0);
    SV("ai-last-failure-x", -1.f);
}

void onDeath(PlayLayer* layer) {
    if (!enabled() || !layer || !layer->m_player1) return;

    auto p = layer->m_player1;
    const float x = p->getPositionX();
    s.failureX = x;
    ++s.failures;
    ++s.attempts;

    // Treat the death position as negative feedback and mutate the local plan.
    // The bot does not repeat the same timing: it tries a nearby earlier/later phase.
    const float previous = V("ai-last-failure-x", -1.f);
    if (previous > 0.f && std::fabs(previous - x) < 18.f) {
        s.jumpWindow += 5.f;
        s.jumpSpacing = std::max(42.f, s.jumpSpacing - 4.f);
    } else {
        s.jumpWindow = std::max(10.f, s.jumpWindow - 1.5f);
        s.jumpSpacing = std::max(44.f, s.jumpSpacing - 1.f);
    }
    SV("ai-last-failure-x", x);
    SI("ai-failures", s.failures);
    SI("ai-attempts", s.attempts);
    ++s.planRevision;

    // Automatic practice checkpoint. It is deliberately placed behind the failure.
    if (s.checkpointX > 0.f && s.checkpointX < x - 20.f) {
        p->setPosition({s.checkpointX, s.checkpointY});
    } else {
        p->setPosition({0.f, p->getPositionY()});
    }

    s.nextDecisionX = std::max(20.f, s.checkpointX + 60.f);
    s.lastX = p->getPositionX();
    s.initialized = true;
    text("LEARNED FROM COLLISION", p->getPositionX(), p->getPositionY(), s.planRevision);
}

void update(PlayLayer* layer, float dt) {
    if (!enabled() || !layer || !layer->m_player1) return;
    auto p = layer->m_player1;

    if (s.layer != layer) {
        if (s.layer) release(s.layer);
        s.layer = layer;
        s.initialized = false;
        s.failures = I("ai-failures", 0);
        s.attempts = I("ai-attempts", 0);
        s.jumpSpacing = std::clamp(V("ai-spacing", 86.f), 44.f, 130.f);
        s.jumpWindow = std::clamp(V("ai-window", 24.f), 8.f, 45.f);
    }

    s.time += dt;
    s.actionTimer += dt;
    const float x = p->getPositionX();
    const float y = p->getPositionY();

    if (!s.initialized) {
        s.lastX = x;
        s.lastY = y;
        s.checkpointX = x;
        s.checkpointY = y;
        s.nextDecisionX = x + 55.f;
        s.initialized = true;
        text("SCANNING RUN", x, y, s.planRevision);
    }

    const float dx = x - s.lastX;
    const float dy = y - s.lastY;
    s.lastX = x;
    s.lastY = y;

    // Advance the automatic checkpoint only after meaningful forward progress.
    // This prevents the AI from checkpointing immediately before a hazard.
    if (dx > 0.01f && x > s.checkpointX + 120.f) {
        s.checkpointX = x - 55.f;
        s.checkpointY = y;
    }

    if (s.holding && s.actionTimer > 0.055f) release(layer);

    // Local model: predict the next required jump from previous successful/failed timing.
    // The plan is deterministic; failures change its phase/spacing rather than introducing randomness.
    if (x + s.jumpWindow >= s.nextDecisionX && x - s.lastActionX > 20.f) {
        tap(layer);
        s.nextDecisionX += s.jumpSpacing;
        text("TESTING JUMP TIMING", x, y, s.planRevision);
    } else if (std::fabs(dy) > 2.f) {
        text(dy > 0.f ? "ASCENDING / VERIFYING" : "DESCENDING / VERIFYING", x, y, s.planRevision);
    } else {
        text("ANALYZING COLLISION HISTORY", x, y, s.planRevision);
    }

    SV("ai-spacing", s.jumpSpacing);
    SV("ai-window", s.jumpWindow);
}
}
