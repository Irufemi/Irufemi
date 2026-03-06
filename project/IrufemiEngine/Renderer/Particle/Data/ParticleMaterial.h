#pragma once

#include "Engine/Core/Math/Vector4.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include <cstdint>

struct ParticleMaterial {
	Vector4 color;
	int32_t useClampSampler = 0; // 0: WRAP, 1: CLAMP
	float pad[3];
	Matrix4x4 uvTransform;
};