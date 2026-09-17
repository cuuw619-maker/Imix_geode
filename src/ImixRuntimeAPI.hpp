#pragma once

#include <cstdint>

namespace ImixRuntime {

// Small, language-neutral frame passed between the game layer and a backend.
struct Frame {
    float playerX = 0.f;
    float playerY = 0.f;
    float velocityY = 0.f;
    float hazardDx = 0.f;
    float hazardDy = 0.f;
    float speed = 0.f;
    int candidate = 2;
};

struct Decision {
    bool shouldJump = false;
    float leadPx = 28.f;
    float confidence = 0.f;
    int phase = 0;
};

enum Capability : std::uint32_t {
    Planning = 1u << 0,
    FailureMemory = 1u << 1,
    Reset = 1u << 2,
    Feedback = 1u << 3,
};

// One small facade for all native backends. Game/input code stays in C++.
unsigned version();
std::uint32_t capabilities();
const char* backendName();
void reset();
void recordFailure(float x, int candidate);
void recordSuccess(float x, int candidate);
Decision plan(const Frame& frame);

// Compatibility overload for existing callers.
inline Decision plan(float playerX, float playerY, float velocityY,
                     float hazardDx, float hazardDy, float speed, int candidate) {
    return plan(Frame{playerX, playerY, velocityY, hazardDx, hazardDy, speed, candidate});
}

}
