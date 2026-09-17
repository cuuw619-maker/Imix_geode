#include "ImixRuntimeAPI.hpp"
#include "../include/imix/runtime.h"
#include <algorithm>
#include <cmath>

#if defined(IMIX_RUST_BACKEND)
extern "C" {
uint32_t imix_rust_version();
uint32_t imix_rust_capabilities();
void imix_rust_reset();
void imix_rust_record_failure(float x, int32_t candidate);
void imix_rust_record_success(float x, int32_t candidate);
ImixRuntimeDecision imix_rust_plan(ImixRuntimeFrame frame);
}
#endif

extern "C" uint32_t imix_runtime_version() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_version();
#else
    return 1;
#endif
}

extern "C" uint32_t imix_runtime_capabilities() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_capabilities();
#else
    return 15;
#endif
}

extern "C" void imix_runtime_reset() {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_reset();
#endif
}

extern "C" void imix_runtime_record_failure(float x, int32_t candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_failure(x, candidate);
#else
    (void)x; (void)candidate;
#endif
}

extern "C" void imix_runtime_record_success(float x, int32_t candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_success(x, candidate);
#else
    (void)x; (void)candidate;
#endif
}

extern "C" ImixRuntimeDecision imix_runtime_plan(ImixRuntimeFrame f) {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_plan(f);
#else
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
        f.velocity_y > 1.f ? 1 : f.velocity_y < -1.f ? 2 : 0
    };
#endif
}

extern "C" const char* imix_runtime_backend() {
#if defined(IMIX_RUST_BACKEND)
    return "Rust";
#else
    return "C++";
#endif
}

namespace ImixRuntime {

unsigned version() { return imix_runtime_version(); }
std::uint32_t capabilities() { return imix_runtime_capabilities(); }
void reset() { imix_runtime_reset(); }
void recordFailure(float x, int candidate) { imix_runtime_record_failure(x, candidate); }
void recordSuccess(float x, int candidate) { imix_runtime_record_success(x, candidate); }
Decision plan(const Frame& f) {
    const auto d = imix_runtime_plan({f.playerX, f.playerY, f.velocityY, f.hazardDx,
                                      f.hazardDy, f.speed, f.candidate});
    return {d.should_jump != 0, d.lead_px, d.confidence, d.phase};
}
const char* backendName() { return imix_runtime_backend(); }

}
