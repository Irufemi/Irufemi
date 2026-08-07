#pragma once

#include <cmath>


namespace Irufemi {
/**
 * @struct Vector2
 * @brief 2次元ベクトル
 */
struct Vector2 final {
	float x;
	float y;

	// 定数
	static const Vector2 zero;
	static const Vector2 one;
	static const Vector2 right;
	static const Vector2 up;

	/**
	 * @brief 添え字演算子
	 * @param index 成分のインデックス (0:x, 1:y)
	 * @return 成分への参照
	 */
	float& operator[](int index);

	/**
	 * @brief 添え字演算子 (const)
	 * @param index 成分のインデックス (0:x, 1:y)
	 * @return 成分の値
	 */
	float operator[](int index) const;

	/** @name 複合代入演算子 */
	/** @{ */
	Vector2& operator+=(const Vector2& rhs);
	Vector2& operator-=(const Vector2& rhs);
	Vector2& operator*=(float s);
	Vector2& operator/=(float s);
	Vector2& operator*=(const Vector2& rhs);
	Vector2& operator/=(const Vector2& rhs);
	/** @} */

	/** @name 比較演算子 */
	/** @{ */
	bool operator==(const Vector2& rhs) const { return x == rhs.x && y == rhs.y; }
	bool operator!=(const Vector2& rhs) const { return !(*this == rhs); }
	/**
	 * @brief Equals を実行する。
	 */
	bool Equals(const Vector2& other, float epsilon = 1e-5f) const {
		return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon;
	}
	/** @} */

	/** @name 数学関数 */
	/** @{ */
	inline float LengthSquared() const { return x * x + y * y; }
	/**
	 * @brief Length を実行する。
	 */
	inline float Length() const { return std::sqrt(LengthSquared()); }
	/**
	 * @brief Normalize を実行する。
	 */
	inline void Normalize() {
		float lenSq = LengthSquared();
		if (lenSq > 0.0f) {
			float invLen = 1.0f / std::sqrt(lenSq);
			x *= invLen;
			y *= invLen;
		}
	}
	/**
	 * @brief Normalized を取得する。
	 * @return 取得された Normalized
	 */
	inline Vector2 GetNormalized() const {
		Vector2 v = *this;
		v.Normalize();
		return v;
	}
	/**
	 * @brief Dot を実行する。
	 */
	inline float Dot(const Vector2& rhs) const { return x * rhs.x + y * rhs.y; }
	/** @} */

	/** @name データアクセサ */
	/** @{ */
	const float* data() const { return &x; }
	/**
	 * @brief data を実行する。
	 */
	float* data() { return &x; }
	/** @} */
};

/** @name 非メンバ演算子 */
/** @{ */

Vector2 operator+(const Vector2& lhs, const Vector2& rhs);
Vector2 operator-(const Vector2& lhs, const Vector2& rhs);
Vector2 operator+(const Vector2& v);
Vector2 operator-(const Vector2& v);
Vector2 operator*(const Vector2& v, float s);
Vector2 operator*(float s, const Vector2& v);
Vector2 operator/(const Vector2& v, float s);

// 要素ごとの乗除算
Vector2 operator*(const Vector2& lhs, const Vector2& rhs);
Vector2 operator/(const Vector2& lhs, const Vector2& rhs);

/** @} */


} // namespace Irufemi
