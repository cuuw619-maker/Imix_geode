#![no_std]

use core::panic::PanicInfo;
use core::sync::atomic::{AtomicI32, AtomicU32, Ordering};

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[no_mangle]
pub extern "C" fn rust_eh_personality() {}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ImixRuntimeFrame {
    pub player_x: f32,
    pub player_y: f32,
    pub velocity_y: f32,
    pub hazard_dx: f32,
    pub hazard_dy: f32,
    pub speed: f32,
    pub candidate: i32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct ImixRuntimeDecision {
    pub should_jump: i32,
    pub lead_px: f32,
    pub confidence: f32,
    pub phase: i32,
}

static FAILURES: AtomicU32 = AtomicU32::new(0);
static LAST_FAILURE_X_BITS: AtomicU32 = AtomicU32::new(u32::MAX);
static LAST_CANDIDATE: AtomicI32 = AtomicI32::new(2);

#[no_mangle]
pub extern "C" fn imix_rust_version() -> u32 { 2 }

#[no_mangle]
pub extern "C" fn imix_rust_capabilities() -> u32 { 15 }

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
pub extern "C" fn imix_rust_record_success(_x: f32, _candidate: i32) {
    let mut current = FAILURES.load(Ordering::Relaxed);
    while current > 0 {
        match FAILURES.compare_exchange_weak(
            current, current - 1, Ordering::Relaxed, Ordering::Relaxed
        ) {
            Ok(_) => break,
            Err(next) => current = next,
        }
    }
}

#[no_mangle]
pub extern "C" fn imix_rust_plan(f: ImixRuntimeFrame) -> ImixRuntimeDecision {
    let failure_count = FAILURES.load(Ordering::Relaxed) as f32;
    let candidate_offset = match f.candidate {
        0 => -6.0,
        1 => -3.0,
        2 => 0.0,
        3 => 3.0,
        _ => 6.0,
    };

    let vertical_penalty = (f.hazard_dy.abs() * 0.045).min(18.0);
    let speed_comp = (f.speed * 0.008).clamp(0.0, 10.0);
    let failure_comp = (failure_count * 1.5).min(10.0);
    let lead = (28.0 + candidate_offset + speed_comp + failure_comp - vertical_penalty)
        .clamp(12.0, 52.0);
    let alignment = (f.hazard_dx - lead).abs();
    let confidence = (1.0 - alignment / 80.0).clamp(0.05, 0.99);
    let should_jump = f.hazard_dx >= 8.0 && f.hazard_dx <= 115.0 && alignment <= 24.0;
    let phase = if f.velocity_y > 1.0 { 1 } else if f.velocity_y < -1.0 { 2 } else { 0 };

    let _ = (f.player_x, f.player_y);
    ImixRuntimeDecision {
        should_jump: if should_jump { 1 } else { 0 },
        lead_px: lead,
        confidence,
        phase,
    }
}
