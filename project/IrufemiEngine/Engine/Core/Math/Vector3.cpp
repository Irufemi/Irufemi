#include "Vector3.h"
#include <stdexcept>
#include <cassert>


namespace Irufemi {
const Vector3 Vector3::zero = { 0.0f, 0.0f, 0.0f };
const Vector3 Vector3::one = { 1.0f, 1.0f, 1.0f };
const Vector3 Vector3::right = { 1.0f, 0.0f, 0.0f };
const Vector3 Vector3::up = { 0.0f, 1.0f, 0.0f };
const Vector3 Vector3::forward = { 0.0f, 0.0f, 1.0f };

float& Vector3::operator[](int index) {
    switch (index) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default: throw std::out_of_range("Vector3 index out of range");
    }
}

float Vector3::operator[](int index) const {
    switch (index) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default: throw std::out_of_range("Vector3 index out of range");
    }
}

Vector3& Vector3::operator+=(const Vector3& rhs) {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

Vector3& Vector3::operator-=(const Vector3& rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Vector3& Vector3::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

Vector3& Vector3::operator/=(float s) {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    x *= inv;
    y *= inv;
    z *= inv;
    return *this;
}

Vector3& Vector3::operator*=(const Vector3& rhs) {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
}

Vector3& Vector3::operator/=(const Vector3& rhs) {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    return *this;
}

Vector3 operator+(const Vector3& lhs, const Vector3& rhs) {
    return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

Vector3 operator-(const Vector3& lhs, const Vector3& rhs) {
    return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

Vector3 operator+(const Vector3& v) {
    return v;
}

Vector3 operator-(const Vector3& v) {
    return { -v.x, -v.y, -v.z };
}

Vector3 operator*(const Vector3& v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

Vector3 operator*(float s, const Vector3& v) {
    return { v.x * s, v.y * s, v.z * s };
}

Vector3 operator/(const Vector3& v, float s) {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return { v.x * inv, v.y * inv, v.z * inv };
}

Vector3 operator*(const Vector3& lhs, const Vector3& rhs) {
    return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

Vector3 operator/(const Vector3& lhs, const Vector3& rhs) {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
    return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z };
}


} // namespace Irufemi
