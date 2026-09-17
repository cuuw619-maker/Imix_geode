#include "ImixRuntimeAPI.hpp"
#include <algorithm>
#include <cmath>

#if defined(IMIX_RUST_BACKEND)
extern "C" {
struct ImixRustDecision { int should_jump; float lead_px; float confidence; int phase; };
unsigned imix_rust_version();
std::uint32_t imix_rust_capabilities();
void imix_rust_reset();
void imix_rust_record_failure(float x, int candidate);
void imix_rust_record_success(float x, int candidate);
ImixRustDecision imix_rust_plan(float player_x, float player_y, float velocity_y,
                                float hazard_dx, float hazard_dy, float speed,
                                int candidate);
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

std::uint32_t capabilities() {
#if defined(IMIX_RUST_BACKEND)
    return imix_rust_capabilities();
#else
    return Planning | FailureMemory | Reset | Feedback;
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

void recordSuccess(float x, int candidate) {
#if defined(IMIX_RUST_BACKEND)
    imix_rust_record_success(x, candidate);
#else
    (void)x; (void)candidate;
#endif
}

Decision plan(const Frame& f) {
#if defined(IMIX_RUST_BACKEND)
    auto d = imix_rust_plan(f.playerX, f.playerY, f.velocityY, f.hazardDx,
                             f.hazardDy, f.speed, f.candidate);
    return {d.should_jump != 0, d.lead_px, d.confidence, d.phase};
#else
    const float offset = f.candidate == 0 ? -6.f : f.candidate == 1 ? -3.f :
                         f.candidate == 3 ? 3.f : f.candidate >= 4 ? 6.f : 0.f;
    const float lead = std::clamp(
        28.f + offset + std::clamp(f.speed * .008f, 0.f, 10.f) -
        std::min(std::fabs(f.hazardDy) * .045f, 18.f), 12.f, 52.f);
    const float alignment = std::fabs(f.hazardDx - lead);
    return {
        f.hazardDx >= 8.f && f.hazardDx <= 115.f && alignment <= 24.f,
        lead,
        std::clamp(1.f - alignment / 80.f, .05f, .99f),
        f.velocityY > 1.f ? 1 : f.velocityY < -1.f ? 2 : 0
    };
#endif
}

const char* backendName() {
#if defined(IMIX_RUST_BACKEND)
    return "Rust";
#else
    return "C++";
#endif
}

}
