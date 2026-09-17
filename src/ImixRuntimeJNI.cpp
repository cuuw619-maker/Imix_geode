#if __has_include(<jni.h>)
#include <jni.h>
#include "../include/imix/runtime.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_imix_runtime_ImixRuntime_nativeVersion(JNIEnv*, jclass) {
    return static_cast<jint>(imix_runtime_version());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_imix_runtime_ImixRuntime_nativeAbiVersion(JNIEnv*, jclass) {
    return static_cast<jint>(imix_runtime_abi_version());
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_imix_runtime_ImixRuntime_nativeCapabilities(JNIEnv*, jclass) {
    return static_cast<jlong>(imix_runtime_capabilities());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_imix_runtime_ImixRuntime_nativeBackend(JNIEnv* env, jclass) {
    return env->NewStringUTF(imix_runtime_backend());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_imix_runtime_ImixRuntime_nativeSelfTest(JNIEnv*, jclass) {
    return static_cast<jint>(imix_runtime_self_test());
}

extern "C" JNIEXPORT void JNICALL
Java_com_imix_runtime_ImixRuntime_nativeReset(JNIEnv*, jclass) {
    imix_runtime_reset();
}

extern "C" JNIEXPORT jfloatArray JNICALL
Java_com_imix_runtime_ImixRuntime_nativePlan(
    JNIEnv* env, jclass, jfloat playerX, jfloat playerY, jfloat velocityY,
    jfloat hazardDx, jfloat hazardDy, jfloat speed, jint candidate,
    jfloat deltaTime, jint flags) {
    ImixRuntimeFrame frame{
        playerX, playerY, velocityY, hazardDx, hazardDy, speed,
        candidate, deltaTime, static_cast<uint32_t>(flags)
    };
    const auto d = imix_runtime_plan(frame);
    jfloatArray out = env->NewFloatArray(7);
    if (!out) return nullptr;
    const jfloat values[] = {
        static_cast<jfloat>(d.should_jump), d.lead_px, d.confidence,
        static_cast<jfloat>(d.phase), d.target_x, d.target_y,
        static_cast<jfloat>(d.flags)
    };
    env->SetFloatArrayRegion(out, 0, 7, values);
    return out;
}
#endif
