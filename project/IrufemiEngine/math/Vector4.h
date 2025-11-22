#pragma once

struct Vector4 {
    float x;
    float y;
    float z;
    float w;

    // 単項演算子
    Vector4 operator+() const;
    Vector4 operator-() const;

    // 代入演算子
    Vector4& operator+=(const Vector4& v);
    Vector4& operator-=(const Vector4& v);
    Vector4& operator*=(float s);
    Vector4& operator/=(float s);
};

// 2項演算子
const Vector4 operator+(const Vector4& v1, const Vector4& v2);
const Vector4 operator-(const Vector4& v1, const Vector4& v2);
const Vector4 operator*(const Vector4& v, float s);
const Vector4 operator*(float s, const Vector4& v);
const Vector4 operator/(const Vector4& v, float s);
