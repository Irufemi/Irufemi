#include "Quaternion.h"

#include <stdexcept>

// --- 添え字演算子 ---

float& Quaternion::operator[](int index) {
	switch (index) {
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	case 3:
		return w;
	default:
		throw std::out_of_range("Quaternion index out of range");
	}
}

float Quaternion::operator[](int index) const {
	switch (index) {
	case 0:
		return x;
	case 1:
		return y;
	case 2:
		return z;
	case 3:
		return w;
	default:
		throw std::out_of_range("Quaternion index out of range");
	}
}

// --- 複合代入演算子 ---

Quaternion& Quaternion::operator+=(const Quaternion& rhs) {
	x += rhs.x;
	y += rhs.y;
	z += rhs.z;
	w += rhs.w;
	return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& rhs) {
	x -= rhs.x;
	y -= rhs.y;
	z -= rhs.z;
	w -= rhs.w;
	return *this;
}

// スカラー乗算の複合代入
Quaternion& Quaternion::operator*=(float s) {
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return *this;
}

// スカラー除算の複合代入
Quaternion& Quaternion::operator/=(float s) {
	x /= s;
	y /= s;
	z /= s;
	w /= s;
	return *this;
}

// --- 非メンバ演算子 ---

Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w }; }

Quaternion operator-(const Quaternion& lhs, const Quaternion& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w }; }

// 単項演算子 + (正号)
Quaternion operator+(const Quaternion& q) { return q; }

// 単項演算子 - (符号反転)
Quaternion operator-(const Quaternion& q) { return { -q.x, -q.y, -q.z, -q.w }; }

// スカラー乗算
Quaternion operator*(const Quaternion& q, float s) { return { q.x * s, q.y * s, q.z * s, q.w * s }; }

// スカラー乗算 (可換性)
Quaternion operator*(float s, const Quaternion& q) {
	// 既存の q * s を呼び出す
	return q * s;
}

// スカラー除算
Quaternion operator/(const Quaternion& q, float s) { return { q.x / s, q.y / s, q.z / s, q.w / s }; }