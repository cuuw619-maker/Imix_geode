#pragma once

namespace ImixRuntime {
struct Decision {
    bool shouldJump = false;
    float leadPx = 28.f;
    float confidence = 0.f;
    int phase = 0;
};

// Stable native boundary for optional Rust (and future language) backends.
// The gameplay layer remains responsible for invoking Geometry Dash input.
unsigned version();
void reset();
void recordFailure(float x, int candidate);
Decision plan(float playerX, float playerY, float velocityY, float hazardDx,
              float hazardDy, float speed, int candidate);
const char* backendName();
}
