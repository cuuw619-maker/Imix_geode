# ImixRust.cmake
# One call enables the Rust native backend for supported Android builds.

function(imix_enable_rust_backend TARGET)
    if (NOT ANDROID OR NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        return()
    endif()

    find_program(IMIX_CARGO cargo)
    if (NOT IMIX_CARGO)
        message(FATAL_ERROR "Imix Rust backend requires Cargo")
    endif()

    set(IMIX_RUST_TARGET "aarch64-linux-android")
    set(IMIX_RUST_DIR "${PROJECT_SOURCE_DIR}/rust")
    set(IMIX_RUST_LIB "${IMIX_RUST_DIR}/target/${IMIX_RUST_TARGET}/release/libimix_rust_core.a")

    add_custom_command(
        OUTPUT "${IMIX_RUST_LIB}"
        COMMAND ${CMAKE_COMMAND} -E env
            "CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=${CMAKE_C_COMPILER}"
            "RUSTFLAGS=-C panic=abort"
            ${IMIX_CARGO} build --manifest-path "${IMIX_RUST_DIR}/Cargo.toml"
            --release --target "${IMIX_RUST_TARGET}"
        WORKING_DIRECTORY "${IMIX_RUST_DIR}"
        DEPENDS
            "${IMIX_RUST_DIR}/Cargo.toml"
            "${IMIX_RUST_DIR}/src/lib.rs"
        COMMENT "Building Imix Rust native backend"
        VERBATIM
    )

    add_custom_target(imix_rust_backend DEPENDS "${IMIX_RUST_LIB}")
    add_dependencies(${TARGET} imix_rust_backend)
    target_compile_definitions(${TARGET} PRIVATE IMIX_RUST_BACKEND=1)
    target_link_libraries(${TARGET} "${IMIX_RUST_LIB}")
endfunction()
