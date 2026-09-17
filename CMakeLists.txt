cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Template VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

if (ANDROID AND CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    find_program(IMIX_CARGO cargo)
    if (NOT IMIX_CARGO)
        message(FATAL_ERROR "Rust/Cargo is required for the Android64 Imix backend")
    endif()

    set(IMIX_RUST_TARGET "aarch64-linux-android")
    set(IMIX_RUST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust")
    set(IMIX_RUST_LIB "${IMIX_RUST_DIR}/target/${IMIX_RUST_TARGET}/release/libimix_rust_core.a")

    add_custom_command(
        OUTPUT "${IMIX_RUST_LIB}"
        COMMAND ${CMAKE_COMMAND} -E env
            "CARGO_TARGET_AARCH64_LINUX_ANDROID_LINKER=${CMAKE_C_COMPILER}"
            "RUSTFLAGS=-C panic=abort"
            ${IMIX_CARGO} build --manifest-path "${IMIX_RUST_DIR}/Cargo.toml" --release --target "${IMIX_RUST_TARGET}"
        WORKING_DIRECTORY "${IMIX_RUST_DIR}"
        DEPENDS "${IMIX_RUST_DIR}/Cargo.toml" "${IMIX_RUST_DIR}/src/lib.rs"
        COMMENT "Building Imix Rust native core (Android64)"
        VERBATIM
    )
    add_custom_target(imix_rust_core DEPENDS "${IMIX_RUST_LIB}")
    add_dependencies(${PROJECT_NAME} imix_rust_core)
    target_compile_definitions(${PROJECT_NAME} PRIVATE IMIX_RUST_BACKEND=1)
    # Geode's setup_geode_mod uses the plain target_link_libraries signature.
    target_link_libraries(${PROJECT_NAME} "${IMIX_RUST_LIB}")
endif()

setup_geode_mod(${PROJECT_NAME})
