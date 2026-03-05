#include "Matrix4x4.h"
#include "function/Math.h"

Matrix4x4& Matrix4x4::operator+=(const Matrix4x4& rhs) {
    *this = Math::Add(*this, rhs);
    return *this;
}

Matrix4x4& Matrix4x4::operator-=(const Matrix4x4& rhs) {
    *this = Math::Subtract(*this, rhs);
    return *this;
}

Matrix4x4& Matrix4x4::operator*=(const Matrix4x4& rhs) {
    *this = Math::Multiply(*this, rhs);
    return *this;
}

Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs) {
    return Math::Add(lhs, rhs);
}

Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs) {
    return Math::Subtract(lhs, rhs);
}

Matrix4x4 operator+(const Matrix4x4& m) {
    return m;
}

Matrix4x4 operator-(const Matrix4x4& m) {
    Matrix4x4 zero{}; // 全要素0で初期化
    return Math::Subtract(zero, m);
}

Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs) {
    return Math::Multiply(lhs, rhs);
}