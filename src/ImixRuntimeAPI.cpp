#include "ImixRuntimeAPI.hpp"
#include <algorithm>
#include <cmath>

#if defined(IMIX_RUST_BACKEND)
extern "C" {
uint32_t imix_rust_version();
uint32_t imix_rust_abi_version();
uint32_t imix_rust_capabilities();
void imix_rust_reset();
void imix_rust_record_failure(float x, float y, int32_t candidate);
void imix_rust_record_success(float x, float y, int32_t candidate);
ImixRuntimeDecision imix_rust_plan(ImixRuntimeFrame frame);
ImixRuntimeStats imix_rust_stats();
}
#endif

namespace {
const char* gLastError = "OK";

ImixRuntimeDecision fallbackPlan(ImixRuntimeFrame f) {
    const float offset = f.candidate == 0 ? -6.f : f.candidate == 1 ? -3.f :
                         f.candidate == 3 ? 3.f : f.candidate >= 4 ? 6.f : 0.f;
    const float lead = std::clamp(
        28.f + offset + std::clamp(f.speed * .008f, 0.f, 10.f) -
        std::min(std::fabs(f.hazard_dy) * .045f, 18.f), 12.f, 52.f);
    const float alignment = std::fabs(f.hazard_dx - lead);
    return {
        f.hazard_dx >= 8.f && f.hazard_dx <= 115.f && alignment <= 24.f,
        lead,
        std::clamp(1.f - alignment / 80.f, .05f, .99f),
        f.velocity_y > 1.f ? 1 : f.velocity_y < -1.f ? 2 : 0,
        f.player_x + f.hazard_dx,
        f.player_y + f.hazard_dy,
        0u
    };
}
}

extern "C" uint32_t imix_runtime_version() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_version();
#else
    return 3;
#endif
}

extern "C" uint32_t imix_runtime_abi_version() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_abi_version();
#else
    return IMIX_RUNTIME_ABI_VERSION;
#endif
}

extern "C" uint32_t imix_runtime_capabilities() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_capabilities();
#else
    return IMIX_CAP_PLANNING | IMIX_CAP_FAILURE_MEMORY | IMIX_CAP_RESET |
           IMIX_CAP_FEEDBACK | IMIX_CAP_TELEMETRY | IMIX_CAP_STATE | IMIX_CAP_KOTLIN;
#endif
}

extern "C" const char* imix_runtime_backend() {
#if defined(IMIX_RUST_BACKEND)
    return "Rust";
#else
    return "C++";
#endif
}

extern "C" const char* imix_runtime_last_error() { return gLastError; }

extern "C" int32_t imix_runtime_self_test() {
    if (imix_runtime_abi_version() != IMIX_RUNTIME_ABI_VERSION) {
        gLastError = "ABI mismatch";
        return IMIX_ERR_ABI_MISMATCH;
    }
    const auto d = imix_runtime_plan({100.f, 100.f, 0.f, 30.f, 0.f, 1.f, 2, 0.016f, 0});
    if (!std::isfinite(d.lead_px) || !std::isfinite(d.confidence)) {
        gLastError = "Invalid planner result";
        return IMIX_ERR_INVALID_ARGUMENT;
    }
    gLastError = "OK";
    return IMIX_OK;
}

extern "C" void imix_runtime_reset() {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_reset();
#endif
    gLastError = "OK";
}

extern "C" void imix_runtime_record_failure(float x, float y, int32_t candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_failure(x, y, candidate);
#else
    (void)x; (void)y; (void)candidate;
#endif
}

extern "C" void imix_runtime_record_success(float x, float y, int32_t candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_success(x, y, candidate);
#else
    (void)x; (void)y; (void)candidate;
#endif
}

extern "C" ImixRuntimeDecision imix_runtime_plan(ImixRuntimeFrame f) {
    if (!std::isfinite(f.player_x) || !std::isfinite(f.player_y) ||
        !std::isfinite(f.velocity_y) || !std::isfinite(f.hazard_dx) ||
        !std::isfinite(f.hazard_dy) || !std::isfinite(f.speed)) {
        gLastError = "Non-finite frame";
        return {0, 28.f, 0.f, 0, f.player_x, f.player_y, 0};
    }
#if defined(IMIX_RUST_BACKEND)
    gLastError = "OK";
    return imix_rust_plan(f);
#else
    gLastError = "OK";
    return fallbackPlan(f);
#endif
}

extern "C" ImixRuntimeStats imix_runtime_stats() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_stats();
#else
    return {};
#endif
}

namespace ImixRuntime {
unsigned version() { return imix_runtime_version(); }
unsigned abiVersion() { return imix_runtime_abi_version(); }
std::uint32_t capabilities() { return imix_runtime_capabilities(); }
const char* backendName() { return imix_runtime_backend(); }
const char* lastError() { return imix_runtime_last_error(); }
bool selfTest() { return imix_runtime_self_test() == IMIX_OK; }
void reset() { imix_runtime_reset(); }
void recordFailure(float x, float y, int candidate) { imix_runtime_record_failure(x, y, candidate); }
void recordSuccess(float x, float y, int candidate) { imix_runtime_record_success(x, y, candidate); }
Decision plan(const Frame& f) {
    const auto d = imix_runtime_plan({f.playerX, f.playerY, f.velocityY, f.hazardDx,
                                      f.hazardDy, f.speed, f.candidate, f.deltaTime, f.flags});
    return {d.should_jump != 0, d.lead_px, d.confidence, d.phase, d.target_x, d.target_y, d.flags};
}
Stats stats() {
    const auto s = imix_runtime_stats();
    return {s.failures, s.successes, s.attempts, s.last_candidate,
            s.last_failure_x, s.last_failure_y, s.confidence};
}
}
