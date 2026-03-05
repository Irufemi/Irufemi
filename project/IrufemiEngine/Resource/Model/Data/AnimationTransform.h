#pragma once

#include "Vector3.h"
#include "Quaternion.h"

struct AnimationTrasform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};