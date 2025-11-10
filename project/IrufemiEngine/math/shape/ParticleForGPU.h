#pragma once

#include "../Vector3.h"
#include "../Vector4.h"
#include "../Transform.h"

struct ParticleForGPU
{
    Matrix4x4 WVP;
    Matrix4x4 world;
    Vector4 color;
};

