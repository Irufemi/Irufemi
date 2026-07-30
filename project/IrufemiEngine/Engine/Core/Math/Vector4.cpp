#include "Vector4.h"
#include <stdexcept>
#include <cassert>


namespace Irufemi {
const Vector4 Vector4::zero = { 0.0f, 0.0f, 0.0f, 0.0f };
const Vector4 Vector4::one = { 1.0f, 1.0f, 1.0f, 1.0f };
const Vector4 Vector4::right = { 1.0f, 0.0f, 0.0f, 0.0f };
const Vector4 Vector4::up = { 0.0f, 1.0f, 0.0f, 0.0f };
const Vector4 Vector4::forward = { 0.0f, 0.0f, 1.0f, 0.0f };

float& Vector4::operator[](int index) {
    switch (index) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    case 3: return w;
    default: throw std::out_of_range("Vector4 index out of range");
    }
}

float Vector4::operator[](int index) const {
    switch (index) {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    case 3: return w;
    default: throw std::out_of_range("Vector4 index out of range");
    }
}

Vector4 Vector4::operator+() const {
    return *this;
}

Vector4 Vector4::operator-() const {
    return {-x, -y, -z, -w};
}

Vector4& Vector4::operator+=(const Vector4& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Vector4& Vector4::operator-=(const Vector4& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Vector4& Vector4::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
    return *this;
}

Vector4& Vector4::operator/=(float s) {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    x *= inv;
    y *= inv;
    z *= inv;
    w *= inv;
    return *this;
}

Vector4& Vector4::operator*=(const Vector4& rhs) {
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    w *= rhs.w;
    return *this;
}

Vector4& Vector4::operator/=(const Vector4& rhs) {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    w /= rhs.w;
    return *this;
}

Vector4 operator+(const Vector4& v1, const Vector4& v2) {
    return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}

Vector4 operator-(const Vector4& v1, const Vector4& v2) {
    return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}

Vector4 operator*(const Vector4& v, float s) {
    return { v.x * s, v.y * s, v.z * s, v.w * s };
}

Vector4 operator*(float s, const Vector4& v) {
    return { v.x * s, v.y * s, v.z * s, v.w * s };
}

Vector4 operator/(const Vector4& v, float s) {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return { v.x * inv, v.y * inv, v.z * inv, v.w * inv };
}

Vector4 operator*(const Vector4& lhs, const Vector4& rhs) {
    return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
}

Vector4 operator/(const Vector4& lhs, const Vector4& rhs) {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
    return { lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w };
}


} // namespace Irufemi
