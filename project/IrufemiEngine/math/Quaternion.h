#pragma once

struct Quaternion {
    float x;
    float y;
    float z;
    float w;

    // 添え字演算子
    float& operator[](int index);
    float operator[](int index) const;

    // 複合代入演算子
    Quaternion& operator+=(const Quaternion& rhs);
    Quaternion& operator-=(const Quaternion& rhs);
    Quaternion& operator*=(float s);
    Quaternion& operator/=(float s);
};

// --- 非メンバ演算子 ---

// 四元数同士の加減算
Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs);
Quaternion operator-(const Quaternion& lhs, const Quaternion& rhs);

// 単項演算子
Quaternion operator+(const Quaternion& q); // 正号
Quaternion operator-(const Quaternion& q); // 符号反転

// スカラーとの乗除算
Quaternion operator*(const Quaternion& q, float s);
Quaternion operator*(float s, const Quaternion& q); // 可換性のため
Quaternion operator/(const Quaternion& q, float s);