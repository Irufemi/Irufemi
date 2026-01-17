#pragma once

struct Matrix4x4 {
    float m[4][4];

    // 複合代入演算子(Math.h の関数を使用)
    Matrix4x4& operator+=(const Matrix4x4& rhs);
    Matrix4x4& operator-=(const Matrix4x4& rhs);
    Matrix4x4& operator*=(const Matrix4x4& rhs);
};

// --- 非メンバ演算子(Math.h の関数を使用) ---
Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs);
Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs);

// 単項演算子
Matrix4x4 operator+(const Matrix4x4& m); // 正号
Matrix4x4 operator-(const Matrix4x4& m); // 符号反転

// 行列同士の積(左から右へ)
Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs);

