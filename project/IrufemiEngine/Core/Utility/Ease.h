#pragma once

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Quaternion.h"

// イージングの種類
enum class EaseType {
    Linear,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint
};

// 指定したイージングタイプで進行度 t を評価する
/**
 * @brief EvaluateEase を実行する。
 */
float EvaluateEase(EaseType type, float t);

// 線形補間
/**
 * @brief Lerp を実行する。
 */
float Lerp(float pos1, float pos2, float t);

// 線形補間
/**
 * @brief Lerp を実行する。
 */
Irufemi::Vector2 Lerp(const Irufemi::Vector2& v1, const Irufemi::Vector2& v2, float t);

// 線形補間
/**
 * @brief Lerp を実行する。
 */
Irufemi::Vector3 Lerp(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

// 線形補間
/**
 * @brief Lerp を実行する。
 */
Irufemi::Vector4 Lerp(const Irufemi::Vector4& v1, const Irufemi::Vector4& v2, float t);

// Irufemi::Quaternion 線形補間(最短経路・正規化)
/**
 * @brief Lerp を実行する。
 */
Irufemi::Quaternion Lerp(const Irufemi::Quaternion& q1, const Irufemi::Quaternion& q2, float t); // 追加

// 線形補間(0~1制限あり)
/**
 * @brief LerpClamped を実行する。
 */
float LerpClamped(float a, float b, float t);

// 線形補間(0~1制限あり)
/**
 * @brief LerpClamped を実行する。
 */
Irufemi::Vector2 LerpClamped(const Irufemi::Vector2& v1, const Irufemi::Vector2& v2, float t);

// 線形補間(0~1制限あり)
/**
 * @brief LerpClamped を実行する。
 */
Irufemi::Vector3 LerpClamped(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

// 球面線形補間
/**
 * @brief Slerp を実行する。
 */
Irufemi::Vector3 Slerp(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

/**
 * @brief EaseInSine を実行する。
 */
float EaseInSine(float num);

/**
 * @brief EaseOutSine を実行する。
 */
float EaseOutSine(float num);

/**
 * @brief EaseInOutSine を実行する。
 */
float EaseInOutSine(float num);

/**
 * @brief EaseInQuad を実行する。
 */
float EaseInQuad(float num);

/**
 * @brief EaseOutQuad を実行する。
 */
float EaseOutQuad(float num);

/**
 * @brief EaseInOutQuad を実行する。
 */
float EaseInOutQuad(float num);

/**
 * @brief EaseInCubic を実行する。
 */
float EaseInCubic(float num);

/**
 * @brief EaseOutCubic を実行する。
 */
float EaseOutCubic(float num);

/**
 * @brief EaseInOutCubic を実行する。
 */
float EaseInOutCubic(float num);

/**
 * @brief EaseInQuart を実行する。
 */
float EaseInQuart(float num);

/**
 * @brief EaseOutQuart を実行する。
 */
float EaseOutQuart(float num);

/**
 * @brief EaseInOutQuart を実行する。
 */
float EaseInOutQuart(float num);

/**
 * @brief EaseInQuint を実行する。
 */
float EaseInQuint(float num);

/**
 * @brief EaseOutQuint を実行する。
 */
float EaseOutQuint(float num);