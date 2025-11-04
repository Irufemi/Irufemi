#pragma once

#include "../math/Vector2.h"
#include "../math/Vector3.h"

// 線形補間
float Lerp(float pos1, float pos2, float t);

// 線形補間
Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t);

// 線形補間
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);

// 線形補間(0~1制限あり)
float LerpClamped(float a, float b, float t);

// 線形補間(0~1制限あり)
Vector2 LerpClamped(const Vector2& v1, const Vector2& v2, float t);

// 線形補間(0~1制限あり)
Vector3 LerpClamped(const Vector3& v1, const Vector3& v2, float t);

// 球面線形補間
Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t);

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