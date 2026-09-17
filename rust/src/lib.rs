#![no_std]

use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ImixRustDecision {
    pub should_jump: i32,
    pub lead_px: f32,
    pub confidence: f32,
    pub phase: i32,
}

static FAILURES: AtomicU32 = AtomicU32::new(0);
static LAST_FAILURE_X_BITS: AtomicU32 = AtomicU32::new(u32::MAX);
static LAST_CANDIDATE: AtomicI32 = AtomicI32::new(2);

#[no_mangle]
pub extern "C" fn imix_rust_version() -> u32 { 1 }

#[no_mangle]
pub extern "C" fn imix_rust_reset() {
    FAILURES.store(0, Ordering::Relaxed);
    LAST_FAILURE_X_BITS.store(u32::MAX, Ordering::Relaxed);
    LAST_CANDIDATE.store(2, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn imix_rust_record_failure(x: f32, candidate: i32) {
    FAILURES.fetch_add(1, Ordering::Relaxed);
    LAST_FAILURE_X_BITS.store(x.to_bits(), Ordering::Relaxed);
    LAST_CANDIDATE.store(candidate, Ordering::Relaxed);
}

#[no_mangle]
pub extern "C" fn imix_rust_plan(
    player_x: f32,
    player_y: f32,
    velocity_y: f32,
    hazard_dx: f32,
    hazard_dy: f32,
    speed: f32,
    candidate: i32,
) -> ImixRustDecision {
    let failure_count = FAILURES.load(Ordering::Relaxed) as f32;
    let candidate_offset = match candidate {
        0 => -6.0,
        1 => -3.0,
        2 => 0.0,
        3 => 3.0,
        _ => 6.0,
    };

    // A deterministic, local trajectory heuristic. Rust does not replace the
    // game hook; it supplies a bounded decision to the C++ control layer.
    let vertical_penalty = (hazard_dy.abs() * 0.045).min(18.0);
    let speed_comp = (speed * 0.008).clamp(0.0, 10.0);
    let failure_comp = (failure_count * 1.5).min(10.0);
    let lead = (28.0 + candidate_offset + speed_comp + failure_comp - vertical_penalty)
        .clamp(12.0, 52.0);

    let alignment = (hazard_dx - lead).abs();
    let confidence = (1.0 - alignment / 80.0).clamp(0.05, 0.99);
    let should_jump = hazard_dx >= 8.0 && hazard_dx <= 115.0 && alignment <= 24.0;
    let phase = if velocity_y > 1.0 { 1 } else if velocity_y < -1.0 { 2 } else { 0 };

    let _ = (player_x, player_y);
    ImixRustDecision {
        should_jump: if should_jump { 1 } else { 0 },
        lead_px: lead,
        confidence,
        phase,
    }
}
