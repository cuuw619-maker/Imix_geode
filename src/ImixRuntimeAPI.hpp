#pragma once

#include <cstdint>
#include "../include/imix/runtime.h"

namespace ImixRuntime {

struct Frame {
    float playerX = 0.f;
    float playerY = 0.f;
    float velocityY = 0.f;
    float hazardDx = 0.f;
    float hazardDy = 0.f;
    float speed = 0.f;
    int candidate = 2;
    float deltaTime = 0.f;
    std::uint32_t flags = 0;
};

struct Decision {
    bool shouldJump = false;
    float leadPx = 28.f;
    float confidence = 0.f;
    int phase = 0;
    float targetX = 0.f;
    float targetY = 0.f;
    std::uint32_t flags = 0;
};

struct Stats {
    std::uint32_t failures = 0;
    std::uint32_t successes = 0;
    std::uint32_t attempts = 0;
    int lastCandidate = 2;
    float lastFailureX = 0.f;
    float lastFailureY = 0.f;
    float confidence = 0.f;
};

enum Capability : std::uint32_t {
    Planning = IMIX_CAP_PLANNING,
    FailureMemory = IMIX_CAP_FAILURE_MEMORY,
    Reset = IMIX_CAP_RESET,
    Feedback = IMIX_CAP_FEEDBACK,
    Telemetry = IMIX_CAP_TELEMETRY,
    State = IMIX_CAP_STATE,
    Kotlin = IMIX_CAP_KOTLIN,
};

enum class Error : std::int32_t {
    Ok = IMIX_OK,
    InvalidArgument = IMIX_ERR_INVALID_ARGUMENT,
    Unsupported = IMIX_ERR_UNSUPPORTED,
    AbiMismatch = IMIX_ERR_ABI_MISMATCH,
};

unsigned version();
unsigned abiVersion();
std::uint32_t capabilities();
const char* backendName();
const char* lastError();
bool selfTest();
void reset();
void recordFailure(float x, float y, int candidate);
void recordSuccess(float x, float y, int candidate);
Decision plan(const Frame& frame);
Stats stats();

inline Decision plan(float playerX, float playerY, float velocityY,
                     float hazardDx, float hazardDy, float speed, int candidate) {
    return plan(Frame{playerX, playerY, velocityY, hazardDx, hazardDy, speed, candidate, 0.f, 0});
}

} // namespace ImixRuntime
