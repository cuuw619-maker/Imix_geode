# Imix Runtime API

Imix uses one stable C ABI as the compatibility boundary. C++ is the native facade, Rust is an optional planner backend, and Kotlin can access the same contract through the JNI bridge.

## Public layers

- `include/imix/runtime.h` — stable C ABI and the single language boundary.
- `src/ImixRuntimeAPI.hpp/.cpp` — C++ facade used by Imix gameplay code.
- `rust/src/lib.rs` — Rust implementation for the planner backend.
- `src/ImixRuntimeJNI.cpp` — optional Android JNI adapter.
- `android/src/main/java/com/imix/runtime/ImixRuntime.kt` — Kotlin facade.
- `cmake/ImixRust.cmake` — Rust/CMake integration.

## ABI

The current ABI version is `3` (`IMIX_RUNTIME_ABI_VERSION`). The public ABI contains only fixed-width integers, floats, POD structs and `extern "C"` functions. No C++ STL types, Geode objects, exceptions or C++ strings cross the boundary.

Primary functions:

```text
imix_runtime_version()
imix_runtime_abi_version()
imix_runtime_capabilities()
imix_runtime_backend()
imix_runtime_last_error()
imix_runtime_self_test()
imix_runtime_reset()
imix_runtime_record_failure(x, y, candidate)
imix_runtime_record_success(x, y, candidate)
imix_runtime_plan(frame)
imix_runtime_stats()
```

`ImixRuntimeFrame` carries player state, nearest hazard geometry, movement speed, timing candidate, frame delta and flags. `ImixRuntimeDecision` returns jump intent, timing lead, confidence, phase and target coordinates.

## Capability bits

```text
1   Planning
2   FailureMemory
4   Reset
8   Feedback
16  Telemetry
32  State
64  Kotlin
```

Consumers should check capabilities instead of assuming an optional feature exists.

## Backend responsibilities

C++ remains responsible for Geometry Dash hooks, scene access, Practice Mode and real player input. The runtime backend analyzes state and returns a decision. It does not expose gameplay memory manipulation through the ABI.

Rust implements the same ABI and can be linked by `ImixRust.cmake`. Without the Rust backend, the C++ implementation remains deterministic and ABI-compatible.

Kotlin uses JNI only as an adapter. Kotlin and Rust do not communicate directly; both consume the same C-compatible contract. This keeps the native boundary stable and avoids making JVM types part of the core runtime.

## Compatibility rules

For additive changes, append fields and preserve existing field order. For incompatible layout or semantic changes, increment the ABI version. Call `imix_runtime_self_test()` before enabling optional runtime features and inspect `imix_runtime_capabilities()` first.
