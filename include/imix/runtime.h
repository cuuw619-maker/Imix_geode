#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ImixRuntimeFrame {
    float player_x;
    float player_y;
    float velocity_y;
    float hazard_dx;
    float hazard_dy;
    float speed;
    int32_t candidate;
} ImixRuntimeFrame;

typedef struct ImixRuntimeDecision {
    int32_t should_jump;
    float lead_px;
    float confidence;
    int32_t phase;
} ImixRuntimeDecision;

uint32_t imix_runtime_version(void);
uint32_t imix_runtime_capabilities(void);
void imix_runtime_reset(void);
void imix_runtime_record_failure(float x, int32_t candidate);
void imix_runtime_record_success(float x, int32_t candidate);
ImixRuntimeDecision imix_runtime_plan(ImixRuntimeFrame frame);
const char* imix_runtime_backend(void);

#ifdef __cplusplus
}
#endif
