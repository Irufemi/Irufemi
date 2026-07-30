#pragma once

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Math/Quaternion.h"

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
float EvaluateEase(EaseType type, float t);

// 線形補間
float Lerp(float pos1, float pos2, float t);

// 線形補間
Irufemi::Vector2 Lerp(const Irufemi::Vector2& v1, const Irufemi::Vector2& v2, float t);

// 線形補間
Irufemi::Vector3 Lerp(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

// 線形補間
Irufemi::Vector4 Lerp(const Irufemi::Vector4& v1, const Irufemi::Vector4& v2, float t);

// Irufemi::Quaternion 線形補間(最短経路・正規化)
Irufemi::Quaternion Lerp(const Irufemi::Quaternion& q1, const Irufemi::Quaternion& q2, float t); // 追加

// 線形補間(0~1制限あり)
float LerpClamped(float a, float b, float t);

// 線形補間(0~1制限あり)
Irufemi::Vector2 LerpClamped(const Irufemi::Vector2& v1, const Irufemi::Vector2& v2, float t);

// 線形補間(0~1制限あり)
Irufemi::Vector3 LerpClamped(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

// 球面線形補間
Irufemi::Vector3 Slerp(const Irufemi::Vector3& v1, const Irufemi::Vector3& v2, float t);

float EaseInSine(float num);

float EaseOutSine(float num);

float EaseInOutSine(float num);

float EaseInQuad(float num);

float EaseOutQuad(float num);

float EaseInOutQuad(float num);

float EaseInCubic(float num);

float EaseOutCubic(float num);

float EaseInOutCubic(float num);

float EaseInQuart(float num);

float EaseOutQuart(float num);

float EaseInOutQuart(float num);

float EaseInQuint(float num);

float EaseOutQuint(float num);