# Imix Runtime API

Imix now has a small, stable native runtime boundary in `src/ImixRuntimeAPI.hpp`.

## Backends

- C++ remains the Geometry Dash/Geode integration layer and owns all game hooks and real input.
- Rust is compiled into the Android64 build as `libimix_rust_core.a` and is called through a C ABI.
- Windows and non-Android targets use the deterministic C++ fallback.

## Rust entry points

```text
imix_rust_version()
imix_rust_reset()
imix_rust_record_failure(x, candidate)
imix_rust_plan(player_x, player_y, velocity_y, hazard_dx, hazard_dy, speed, candidate)
```

The Rust backend is deliberately deterministic and local. It does not use HTTP, API keys, cloud inference, or a JVM. The returned decision is advisory; C++ remains responsible for invoking `PlayLayer::handleButton`, so the backend cannot silently replace real gameplay with no-clip or invulnerability.

## Adding another native language

A future backend can implement the same C ABI and be selected by the C++ bridge. Kotlin is not used as the core Geode runtime because the mod itself is a native shared library; Kotlin would require a separate Android/JVM layer and JNI boundary. That can be added later without changing the public C++ decision contract.
