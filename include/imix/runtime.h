#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IMIX_RUNTIME_ABI_VERSION 3u
#define IMIX_RUNTIME_MAX_VERSION 3u

typedef struct ImixRuntimeFrame {
    float player_x;
    float player_y;
    float velocity_y;
    float hazard_dx;
    float hazard_dy;
    float speed;
    int32_t candidate;
    float delta_time;
    uint32_t flags;
} ImixRuntimeFrame;

typedef struct ImixRuntimeDecision {
    int32_t should_jump;
    float lead_px;
    float confidence;
    int32_t phase;
    float target_x;
    float target_y;
    uint32_t flags;
} ImixRuntimeDecision;

typedef struct ImixRuntimeStats {
    uint32_t failures;
    uint32_t successes;
    uint32_t attempts;
    int32_t last_candidate;
    float last_failure_x;
    float last_failure_y;
    float confidence;
} ImixRuntimeStats;

typedef enum ImixRuntimeCapability {
    IMIX_CAP_PLANNING       = 1u << 0,
    IMIX_CAP_FAILURE_MEMORY = 1u << 1,
    IMIX_CAP_RESET          = 1u << 2,
    IMIX_CAP_FEEDBACK       = 1u << 3,
    IMIX_CAP_TELEMETRY      = 1u << 4,
    IMIX_CAP_STATE          = 1u << 5,
    IMIX_CAP_KOTLIN         = 1u << 6,
} ImixRuntimeCapability;

typedef enum ImixRuntimeError {
    IMIX_OK = 0,
    IMIX_ERR_INVALID_ARGUMENT = 1,
    IMIX_ERR_UNSUPPORTED = 2,
    IMIX_ERR_ABI_MISMATCH = 3,
} ImixRuntimeError;

uint32_t imix_runtime_version(void);
uint32_t imix_runtime_abi_version(void);
uint32_t imix_runtime_capabilities(void);
const char* imix_runtime_backend(void);
const char* imix_runtime_last_error(void);
int32_t imix_runtime_self_test(void);

void imix_runtime_reset(void);
void imix_runtime_record_failure(float x, float y, int32_t candidate);
void imix_runtime_record_success(float x, float y, int32_t candidate);
ImixRuntimeDecision imix_runtime_plan(ImixRuntimeFrame frame);
ImixRuntimeStats imix_runtime_stats(void);

#ifdef __cplusplus
}
#endif
