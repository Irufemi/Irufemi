#include "Vector3.h"

#include <stdexcept>


// --- 添え字演算子 ---

float& Vector3::operator[](int index) {
    switch (index) {
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    default:
        throw std::out_of_range("Vector3 index out of range");
    }
}

float Vector3::operator[](int index) const {
    switch (index) {
    case 0:
        return x;
    case 1:
        return y;
    case 2:
        return z;
    default:
        throw std::out_of_range("Vector3 index out of range");
    }
}

// --- 複合代入演算子 ---

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

// 【追加】スカラー乗算の複合代入
Vector3& Vector3::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

// 【追加】スカラー除算の複合代入
Vector3& Vector3::operator/=(float s) {
    x /= s;
    y /= s;
    z /= s;
    return *this;
}

// --- 非メンバ演算子 ---

Vector3 operator+(const Vector3& lhs, const Vector3& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z }; }

Vector3 operator-(const Vector3& lhs, const Vector3& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }

// 【追加】単項演算子 + (正号)
Vector3 operator+(const Vector3& v) { return v; }

// 【追加】単項演算子 - (符号反転)
Vector3 operator-(const Vector3& v) { return { -v.x, -v.y, -v.z }; }

// 【追加】スカラー乗算
Vector3 operator*(const Vector3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }

// 【追加】スカラー乗算 (可換性)
Vector3 operator*(float s, const Vector3& v) {
    // 既存の v * s を呼び出す
    return v * s;
}

// 【追加】スカラー除算
Vector3 operator/(const Vector3& v, float s) { return { v.x / s, v.y / s, v.z / s }; }
