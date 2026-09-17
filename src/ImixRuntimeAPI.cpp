#include "ImixRuntimeAPI.hpp"
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>

#if defined(IMIX_RUST_BACKEND)
extern "C" {
struct ImixRustDecision { int should_jump; float lead_px; float confidence; int phase; };
unsigned imix_rust_version();
void imix_rust_reset();
void imix_rust_record_failure(float x, int candidate);
ImixRustDecision imix_rust_plan(float player_x, float player_y, float velocity_y, float hazard_dx, float hazard_dy, float speed, int candidate);
}
#endif

namespace ImixRuntime {
unsigned version() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_version();
#else
    return 0;
#endif
}
void reset() {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_reset();
#endif
}
void recordFailure(float x, int candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_failure(x, candidate);
#else
    (void)x; (void)candidate;
#endif
}
Decision plan(float playerX, float playerY, float velocityY, float hazardDx, float hazardDy, float speed, int candidate) {
#if defined(IMIX_RUST_BACKEND)
    auto d = imix_rust_plan(playerX, playerY, velocityY, hazardDx, hazardDy, speed, candidate);
    return {d.should_jump != 0, d.lead_px, d.confidence, d.phase};
#else
    float offset = candidate == 0 ? -6.f : candidate == 1 ? -3.f : candidate == 3 ? 3.f : candidate >= 4 ? 6.f : 0.f;
    float lead = std::clamp(28.f + offset + std::clamp(speed * .008f, 0.f, 10.f) - std::min(std::fabs(hazardDy) * .045f, 18.f), 12.f, 52.f);
    float alignment = std::fabs(hazardDx - lead);
    return {hazardDx >= 8.f && hazardDx <= 115.f && alignment <= 24.f, lead, std::clamp(1.f - alignment / 80.f, .05f, .99f), velocityY > 1.f ? 1 : velocityY < -1.f ? 2 : 0};
#endif
}
const char* backendName() {
#if defined(IMIX_RUST_BACKEND)
    return "Rust native core";
#else
    return "C++ fallback";
#endif
}
}
