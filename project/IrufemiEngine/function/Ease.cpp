#include "Ease.h"

#include "Math.h"
#include <algorithm>
#include <cmath>
#include <numbers>

using namespace Math;

float Lerp(float pos1, float pos2, float t	) {
	float result;
	result = (1.0f - t) * pos1 + t * pos2;
	return result;
}

// 線形補間
Vector2 Lerp(const Vector2& v1, const Vector2& v2, float t){
	return Add(Multiply(1.0f - t, v1), Multiply(t, v2));
}

// 線形補間
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t) { 
	return Add(Multiply(1.0f - t, v1), Multiply(t, v2));
}

// 線形補間(0~1制限あり)
float LerpClamped(float a, float b, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return Lerp(a, b, t);
}

// 線形補間(0~1制限あり)
Vector2 LerpClamped(const Vector2& v1, const Vector2& v2, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return Lerp(v1, v2, t);
}

// 線形補間(0~1制限あり)
Vector3 LerpClamped(const Vector3& v1, const Vector3& v2, float t) {
	t = std::clamp(t, 0.0f, 1.0f);
	return Lerp(v1, v2, t);
}

// 球面線形補間
Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t) {
	// --- ユーティリティ ---
	auto len = [](const Vector3& v) -> float { return std::sqrt(Dot(v, v)); };
	auto safeNormalize = [](const Vector3& v) -> Vector3 {
		float l = std::sqrt(Dot(v, v));
		if (l <= 1e-8f)
			return Vector3{0, 0, 0};
		return Multiply(1.0f / l, v);
	};

	// 長さ
	float l1 = len(v1);
	float l2 = len(v2);

	// どちらもゼロ長
	if (l1 <= 1e-8f && l2 <= 1e-8f)
		return Vector3{0, 0, 0};
	// 一方がゼロ長：もう片方の方向へ長さだけLerp
	if (l1 <= 1e-8f)
		return Multiply(std::clamp(t, 0.0f, 1.0f) * l2, safeNormalize(v2));
	if (l2 <= 1e-8f)
		return Multiply((1.0f - std::clamp(t, 0.0f, 1.0f)) * l1, safeNormalize(v1));

	// 単位方向
	Vector3 u1 = Multiply(1.0f / l1, v1);
	Vector3 u2 = Multiply(1.0f / l2, v2);

	// 角度
	float d = std::clamp(Dot(u1, u2), -1.0f, 1.0f);
	float theta = std::acos(d);
	float length = Lerp(l1, l2, t); // 長さは線形補間

	// 角度が極小なら nlerp 的に処理（sinθ ≒ 0 回避）
	if (theta < 1e-5f) {
		Vector3 dir = safeNormalize(Add(Multiply(1.0f - t, u1), Multiply(t, u2)));
		return Multiply(length, dir);
	}

	// 180°付近（無数の経路がある）対策：任意の直交方向を選んで回す
	if (std::abs(std::numbers::pi_v<float> - theta) < 1e-4f) {
		// u1 とほぼ平行でない基準軸を選ぶ
		Vector3 axis = (std::abs(u1.x) < 0.9f) ? Vector3{1, 0, 0} : Vector3{0, 1, 0};
		Vector3 ortho = safeNormalize(Cross(u1, axis));
		Vector3 vperp = safeNormalize(Cross(ortho, u1)); // u1 に直交する単位ベクトル
		Vector3 dir = Add(Multiply(std::cos(std::numbers::pi_v<float> * t), u1), Multiply(std::sin(std::numbers::pi_v<float> * t), vperp));
		return Multiply(length, dir);
	}

	// 通常のSlerp
	float sinTheta = std::sin(theta);
	float a = std::sin((1.0f - t) * theta) / sinTheta;
	float b = std::sin(t * theta) / sinTheta;
	Vector3 dir = Add(Multiply(a, u1), Multiply(b, u2));
	return Multiply(length, dir);
}

float EaseInSine(float num) { return 1.0f - std::cosf((num * std::numbers::pi_v<float> / 2.0f)); }

float EaseOutSine(float num) { return std::sinf(num * std::numbers::pi_v<float> / 2.0f); }

float EaseInOutSine(float num) { return -(std::cosf(std::numbers::pi_v<float> * num) - 1.0f) / 2.0f; }

float EaseInQuad(float num) { return num * num; }

float EaseOutQuad(float num) { return 1.0f - (1.0f - num) * (1.0f - num); }

float EaseInOutQuad(float num) { return num < 0.5f ? 2.0f * num * num : 1.0f - std::powf(-2.0f * num + 2.0f, 2.0f) / 2.0f; }

float EaseInCubic(float num) { return num * num * num; }

float EaseOutCubic(float num) { return 1.0f - std::powf(1.0f - num, 3.0f); }

float EaseInOutCubic(float num) { return num < 0.5f ? 4.0f * num * num * num : 1.0f - std::powf(-2.0f * num + 2.0f, 3.0f) / 2.0f; }

float EaseInQuart(float num) { return num * num * num * num; }

float EaseOutQuart(float num) { return 1.0f - std::powf(1.0f - num, 4.0f); }

float EaseInOutQuart(float num) { return num < 0.5f ? 8.0f * num * num * num * num : 1.0f - std::powf(-2.0f * num + 2.0f, 4.0f) / 2.0f; }

float EaseInQuint(float num) { return num * num * num * num * num; }

float EaseOutQuint(float num) { return 1.0f - std::powf(1.0f - num, 5.0f); }

float EaseInOutQuint(float num) { return num < 0.5f ? 16.0f * num * num * num * num * num : 1.0f - std::powf(-2.0f * num + 2.0f, 5.0f) / 2; }
