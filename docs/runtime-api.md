# Imix Runtime API

Imix uses one small native runtime contract instead of exposing backend-specific code to gameplay modules.

## Public layers

- `include/imix/runtime.h` — stable C ABI. This is the language boundary.
- `src/ImixRuntimeAPI.hpp/.cpp` — compact C++ facade used by Imix gameplay code.
- `rust/src/lib.rs` — Rust implementation for Android64.
- `cmake/ImixRust.cmake` — reusable CMake integration; the root CMake only needs `include(...)` + `imix_enable_rust_backend(...)`.

The C ABI intentionally contains only plain C structs, integers, floats and functions. That makes it suitable for C++, Rust, a future Kotlin/JNI bridge, or another native module without exposing Geode/Cocos2d types.

## Contract

```text
imix_runtime_version()
imix_runtime_capabilities()
imix_runtime_reset()
imix_runtime_record_failure(x, candidate)
imix_runtime_record_success(x, candidate)
imix_runtime_plan(frame)
imix_runtime_backend()
```

`ImixRuntimeFrame` carries player state, nearest hazard geometry, movement speed and the trajectory candidate. `ImixRuntimeDecision` returns the proposed jump, timing lead, confidence and movement phase.

Capability bits:

```text
1 = Planning
2 = FailureMemory
4 = Reset
8 = Feedback
```

## Responsibilities

C++ remains responsible for Geometry Dash hooks, scene access, Practice Mode and real player input. The runtime backend only analyzes state and returns a decision. It cannot implement no-clip, invulnerability or teleportation through this interface.

Rust is built as a static library and linked into Android64. Windows/non-Android builds retain the deterministic C++ implementation. No HTTP, API key, cloud inference or JVM is required.

## Adding a backend

Implement the same C ABI and connect it in the runtime bridge. Kotlin can be added later as an Android/JNI adapter while preserving the exact same public contract; it should not become the core Geode native runtime.
