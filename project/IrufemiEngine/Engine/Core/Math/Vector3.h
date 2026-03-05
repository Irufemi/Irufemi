#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;

	// 添え字演算子
	float& operator[](int index);
	float operator[](int index) const;

	// 複合代入演算子
	Vector3& operator+=(const Vector3& rhs);
	Vector3& operator-=(const Vector3& rhs);
	Vector3& operator*=(float s);
	Vector3& operator/=(float s);
};

// --- 非メンバ演算子 ---

// ベクトル同士の加減算
Vector3 operator+(const Vector3& lhs, const Vector3& rhs);
Vector3 operator-(const Vector3& lhs, const Vector3& rhs);

// 単項演算子
Vector3 operator+(const Vector3& v); // 正号
Vector3 operator-(const Vector3& v); // 符号反転

// スカラーとの乗除算
Vector3 operator*(const Vector3& v, float s);
Vector3 operator*(float s, const Vector3& v); // 可換性のため
Vector3 operator/(const Vector3& v, float s);