#pragma once

/// <summary>
/// 2次元ベクトル
/// </summary>
struct Vector2 final {
	float x;
	float y;

	// 添え字演算子
	float& operator[](int index);
	float operator[](int index) const;

	// 複合代入演算子
	Vector2& operator+=(const Vector2& rhs);
	Vector2& operator-=(const Vector2& rhs);
	Vector2& operator*=(float s);
	Vector2& operator/=(float s);
};

// --- 非メンバ演算子 ---

// ベクトル同士の加減算
Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);

// 単項演算子
Vector2 operator+(const Vector2& v); // 正号
Vector2 operator-(const Vector2& v); // 符号反転

// スカラーとの乗除算
Vector2 operator*(const Vector2& v, float s);
Vector2 operator*(float s, const Vector2& v); // 可換性のため
Vector2 operator/(const Vector2& v, float s);