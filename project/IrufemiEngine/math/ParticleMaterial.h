#pragma once

#include "Vector4.h"
#include "Matrix4x4.h"
#include <cstdint>

struct ParticleMaterial {
	Vector4 color;
	int32_t enableLighting = false;
	int32_t hasTexture = true;
	int32_t lightingMode = 2;
	int32_t useClampSampler = 0; // 0: WRAP, 1: CLAMP
	Matrix4x4 uvTransform;
	float shininess = 1.0f;
};