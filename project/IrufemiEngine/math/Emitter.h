#pragma once

#include "Transform.h"
#include "Vector3.h"
#include <cstdint>

struct Emitter {
	Transform transform; //!< エミッタのトランスフォーム
	uint32_t count; //!< 発生数
	float frequency; //!< 発生頻度
	float frequencyTime; //!< 頻度用時刻
	Vector3 area; //!< 発生領域のサイズ
	Vector3 velocityMin; //!< 初速の最小値
	Vector3 velocityMax; //!< 初速の最大値
};