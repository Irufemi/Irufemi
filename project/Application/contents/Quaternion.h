#pragma once
#include "math/Vector3.h"

struct Quaternion {
  float x, y, z, w;
};

Quaternion Normalize(const Quaternion &q);
Quaternion Multiply(const Quaternion &a, const Quaternion &b);
Quaternion FromAxisAngle(const Vector3 &axis, float angle);
Vector3 QuaternionToEuler(const Quaternion &q);
Vector3 RotateByQuaternion(const Vector3 &v, const Quaternion &q);