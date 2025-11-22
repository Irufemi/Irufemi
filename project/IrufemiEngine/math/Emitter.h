#pragma once

#include "Transform.h"
#include "Vector3.h"
#include <cstdint>

enum class ParticleColorMode {
	kNone,   // startColor/endColorを直接使用
	kRandom, // 完全なランダム色
	kRed,    // 赤系のランダム色
	kGreen,  // 緑系のランダム色
	kBlue,   // 青系のランダム色
};

struct Emitter {
	Transform transform; //!< エミッタのトランスフォーム
	uint32_t count; //!< 発生数
	float frequency = 0.5f; //!< 発生頻度
	float frequencyTime; //!< 頻度用時刻
	Vector3 area; //!< 発生領域のサイズ
	Vector3 velocityMin; //!< 初速の最小値
	Vector3 velocityMax; //!< 初速の最大値
	Vector3 startScale = { 1.0f, 1.0f, 1.0f };
	Vector3 endScale = { 1.0f, 1.0f, 1.0f };
	Vector4 startColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	Vector4 endColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	ParticleColorMode colorMode = ParticleColorMode::kRandom;
};