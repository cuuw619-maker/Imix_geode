package com.imix.runtime

/**
 * Stable Kotlin-facing facade for the native Imix runtime.
 * The native ABI remains C-compatible so Kotlin, Rust and C++ share one contract.
 */
object ImixRuntime {
    data class Decision(
        val shouldJump: Boolean,
        val leadPx: Float,
        val confidence: Float,
        val phase: Int,
        val targetX: Float,
        val targetY: Float,
        val flags: Int,
    )

    const val CAP_PLANNING = 1L shl 0
    const val CAP_FAILURE_MEMORY = 1L shl 1
    const val CAP_RESET = 1L shl 2
    const val CAP_FEEDBACK = 1L shl 3
    const val CAP_TELEMETRY = 1L shl 4
    const val CAP_STATE = 1L shl 5
    const val CAP_KOTLIN = 1L shl 6

    @JvmStatic external fun nativeVersion(): Int
    @JvmStatic external fun nativeAbiVersion(): Int
    @JvmStatic external fun nativeCapabilities(): Long
    @JvmStatic external fun nativeBackend(): String
    @JvmStatic external fun nativeSelfTest(): Int
    @JvmStatic external fun nativeReset()

    @JvmStatic
    fun plan(
        playerX: Float,
        playerY: Float,
        velocityY: Float,
        hazardDx: Float,
        hazardDy: Float,
        speed: Float,
        candidate: Int,
        deltaTime: Float = 0f,
        flags: Int = 0,
    ): Decision {
        val v = nativePlan(playerX, playerY, velocityY, hazardDx, hazardDy, speed, candidate, deltaTime, flags)
        require(v.size >= 7) { "Invalid Imix runtime decision" }
        return Decision(v[0] != 0f, v[1], v[2], v[3].toInt(), v[4], v[5], v[6].toInt())
    }

    @JvmStatic
    private external fun nativePlan(
        playerX: Float,
        playerY: Float,
        velocityY: Float,
        hazardDx: Float,
        hazardDy: Float,
        speed: Float,
        candidate: Int,
        deltaTime: Float,
        flags: Int,
    ): FloatArray
}
