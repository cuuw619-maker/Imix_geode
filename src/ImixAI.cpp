#include "ImixAI.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace geode::prelude;

namespace ImixAI {
namespace {
    bool F(const char* k, bool d = false) { return Mod::get()->getSavedValue<bool>(k, d); }
    float V(const char* k, float d) { return Mod::get()->getSavedValue<float>(k, d); }
    void SV(const char* k, float v) { Mod::get()->setSavedValue(k, v); }
    int I(const char* k, int d) { return Mod::get()->getSavedValue<int>(k, d); }
    void SI(const char* k, int v) { Mod::get()->setSavedValue(k, v); }
    std::string S(const char* k, const std::string& d = {}) { return Mod::get()->getSavedValue<std::string>(k, d); }
    void SS(const char* k, const std::string& v) { Mod::get()->setSavedValue(k, v); }

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
        float lastFailureY = 0.f;
        int failures = 0;
        int attempts = 0;
        int planRevision = 0;
        int candidate = 1;
        int segmentFailures = 0;
        int segmentSuccesses = 0;
        int errorBudget = 4;
        bool holding = false;
        bool initialized = false;
        bool searching = false;
        const char* phase = "BOOT";
    } s;

    int budget() {
        // Keep exploration bounded. Repeated failures in the same area unlock only a
        // small extra search allowance; the bot never enters an unbounded retry loop.
        const int local = std::min(2, std::max(0, s.segmentFailures / 2));
        return std::min(6, 4 + local);
    }

    const char* candidateName() {
        switch (s.candidate) {
            case 0: return "EARLY";
            case 2: return "LATE";
            default: return "CENTER";
        }
    }

    void remember(const char* reason, float x, float y) {
        char entry[180];
        std::snprintf(entry, sizeof(entry), "%s@%.1f,%.1f[%s]", reason, x, y, candidateName());
        auto history = S("ai-memory", "");
        if (!history.empty()) history += "|";
        history += entry;
        // Keep the persistent memory useful and bounded.
        if (history.size() > 1900) history.erase(0, history.size() - 1900);
        SS("ai-memory", history);
    }

    void logState(const char* phase, float x, float y) {
        s.phase = phase;
        log::info("[ImixAI][SEARCH] phase={} x={:.1f} y={:.1f} plan={} candidate={} failures={} budget={} checkpoint={:.1f}",
            phase, x, y, s.planRevision, candidateName(), s.segmentFailures, s.errorBudget, s.checkpointX);
    }

    void text(const char* phase, float x, float y, float vy) {
        if (!s.overlay) return;
        const float progress = s.errorBudget > 0 ? std::min(1.f, static_cast<float>(s.segmentFailures) / s.errorBudget) : 1.f;
        const int bars = static_cast<int>(progress * 10.f + 0.5f);
        char bar[16] = {};
        for (int i = 0; i < 10; ++i) bar[i] = i < bars ? '#' : '-';
        char buf[520];
        std::snprintf(buf, sizeof(buf),
            "IMIX AI  |  %s\n"
            "X %.0f  Y %.0f  VY %.1f   PLAN %d\n"
            "SEARCH  [%s]  candidates: EARLY / CENTER / LATE\n"
            "NEXT %.0f  GAP %.1f  WINDOW %.1f\n"
            "CHECKPOINT %.0f  |  ERRORS %d/%d  [%s]\n"
            "MEMORY %d  |  SEG OK %d  |  LOG: ImixAI",
            phase, x, y, vy, s.planRevision, candidateName(), s.nextDecisionX,
            s.jumpSpacing, s.jumpWindow, s.checkpointX, s.segmentFailures,
            s.errorBudget, bar, s.attempts, s.segmentSuccesses);
        s.overlay->setString(buf);
    }

    void tap(PlayLayer* layer) {
        if (!layer) return;
        layer->handleButton(true, 1, true);
        s.holding = true;
        s.actionTimer = 0.f;
        s.lastActionX = layer->m_player1 ? layer->m_player1->getPositionX() : s.lastActionX;
        log::info("[ImixAI][ACTION] jump candidate={} x={:.1f} window={:.1f}", candidateName(), s.lastActionX, s.jumpWindow);
    }

    void release(PlayLayer* layer) {
        if (!layer || !s.holding) return;
        layer->handleButton(false, 1, true);
        s.holding = false;
    }

    void chooseNextCandidate() {
        // Deterministic local search: center -> early -> late -> center, then tighten.
        // No random input spam is used.
        if (s.candidate == 1) s.candidate = 0;
        else if (s.candidate == 0) s.candidate = 2;
        else s.candidate = 1;
        ++s.planRevision;

        const float phase = s.candidate == 0 ? -1.f : s.candidate == 2 ? 1.f : 0.f;
        s.jumpWindow = std::clamp(22.f + phase * 4.f + s.segmentFailures * 1.5f, 8.f, 38.f);
        s.jumpSpacing = std::clamp(s.jumpSpacing + phase * 2.f - 0.5f, 44.f, 120.f);
    }
}

bool enabled() {
    return F("ai-enabled", false);
}

CCLabelTTF* createOverlay() {
    auto l = CCLabelTTF::create("IMIX AI | booting search", "sans-serif", 9.f);
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
    SI("ai-segment-failures", 0);
    SI("ai-segment-successes", 0);
    SV("ai-last-failure-x", -1.f);
    SS("ai-memory", "");
    log::info("[ImixAI][RESET] persistent error memory and planner state cleared");
}

void onDeath(PlayLayer* layer) {
    if (!enabled() || !layer || !layer->m_player1) return;

    auto p = layer->m_player1;
    const float x = p->getPositionX();
    const float y = p->getPositionY();
    const float previous = V("ai-last-failure-x", -1.f);

    ++s.failures;
    ++s.attempts;
    ++s.segmentFailures;
    s.failureX = x;
    s.lastFailureY = y;
    s.errorBudget = budget();

    // A failure is a constraint, not a reason to spam inputs. Nearby repeated
    // failures widen the search around the collision point; distant failures
    // preserve the learned timing and only shift it slightly.
    const bool sameRegion = previous > 0.f && std::fabs(previous - x) < 24.f;
    if (sameRegion) {
        chooseNextCandidate();
        s.jumpWindow = std::clamp(s.jumpWindow + 2.5f, 8.f, 40.f);
    } else {
        s.jumpWindow = std::clamp(s.jumpWindow - 1.f, 8.f, 40.f);
    }

    SV("ai-last-failure-x", x);
    SV("ai-last-failure-y", y);
    SV("ai-spacing", s.jumpSpacing);
    SV("ai-window", s.jumpWindow);
    SI("ai-failures", s.failures);
    SI("ai-attempts", s.attempts);
    SI("ai-segment-failures", s.segmentFailures);
    SI("ai-segment-successes", s.segmentSuccesses);

    remember("FAIL", x, y);
    log::warn("[ImixAI][FAIL] x={:.1f} y={:.1f} same-region={} -> candidate={} plan={} errors={}/{}",
        x, y, sameRegion, candidateName(), s.planRevision, s.segmentFailures, s.errorBudget);

    // Automatic rollback. Keep the checkpoint meaningfully behind the failure.
    // The existing practice-shield prevents the normal death flow from ending the run.
    if (s.checkpointX > 0.f && s.checkpointX < x - 20.f) {
        p->setPosition({s.checkpointX, s.checkpointY});
        log::info("[ImixAI][ROLLBACK] to checkpoint x={:.1f} y={:.1f}", s.checkpointX, s.checkpointY);
    } else {
        p->setPosition({0.f, p->getPositionY()});
        log::info("[ImixAI][ROLLBACK] no safe checkpoint yet; returned to x=0");
    }

    s.nextDecisionX = std::max(20.f, s.checkpointX + 45.f + s.jumpSpacing * 0.35f);
    s.lastX = p->getPositionX();
    s.initialized = true;
    s.searching = true;
    logState("LEARNED FAILURE -> SEARCHING ALTERNATIVE", p->getPositionX(), p->getPositionY());
    text("FAILURE MEMORY -> NEW PLAN", p->getPositionX(), p->getPositionY(), 0.f);
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
        s.segmentFailures = I("ai-segment-failures", 0);
        s.segmentSuccesses = I("ai-segment-successes", 0);
        s.jumpSpacing = std::clamp(V("ai-spacing", 86.f), 44.f, 120.f);
        s.jumpWindow = std::clamp(V("ai-window", 24.f), 8.f, 40.f);
        s.errorBudget = budget();
        log::info("[ImixAI][BOOT] adaptive search online; bounded error budget={}", s.errorBudget);
    }

    s.time += dt;
    s.actionTimer += dt;
    const float x = p->getPositionX();
    const float y = p->getPositionY();
    const float vy = static_cast<float>(p->m_yVelocity);

    if (!s.initialized) {
        s.lastX = x;
        s.lastY = y;
        s.checkpointX = x;
        s.checkpointY = y;
        s.nextDecisionX = x + 55.f;
        s.errorBudget = budget();
        s.initialized = true;
        logState("SCANNING RUN", x, y);
        text("SCANNING RUN", x, y, vy);
    }

    const float dx = x - s.lastX;
    const float dy = y - s.lastY;
    s.lastX = x;
    s.lastY = y;

    // Checkpoints move forward only after verified progress. They never jump
    // directly onto the current collision area.
    if (dx > 0.01f && x > s.checkpointX + 120.f) {
        s.checkpointX = x - 55.f;
        s.checkpointY = y;
        ++s.segmentSuccesses;
        s.segmentFailures = 0;
        s.errorBudget = budget();
        log::info("[ImixAI][CHECKPOINT] committed x={:.1f} y={:.1f}; local error budget reset to {}",
            s.checkpointX, s.checkpointY, s.errorBudget);
        text("SAFE PROGRESS -> CHECKPOINT", x, y, vy);
    }

    if (s.holding && s.actionTimer > 0.055f) release(layer);

    // The planner uses a small deterministic candidate set. Each failure changes
    // the candidate/timing model instead of issuing a burst of random jumps.
    if (x + s.jumpWindow >= s.nextDecisionX && x - s.lastActionX > 20.f) {
        s.searching = true;
        tap(layer);
        s.nextDecisionX += s.jumpSpacing;
        logState("TESTING CANDIDATE TIMING", x, y);
        text("SEARCHING: TEST CANDIDATE", x, y, vy);
    } else if (s.searching && std::fabs(dy) > 2.f) {
        text(vy > 0.f ? "VERIFYING ASCENT" : "VERIFYING DESCENT", x, y, vy);
    } else {
        text("ANALYZING HISTORY / PREDICTING", x, y, vy);
    }

    SV("ai-spacing", s.jumpSpacing);
    SV("ai-window", s.jumpWindow);
}
}
