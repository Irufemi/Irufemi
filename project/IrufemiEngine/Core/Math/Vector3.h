#pragma once

#include <cmath>

namespace Irufemi {
/**
 * @struct Vector3
 * @brief 3次元ベクトル
 */
struct Vector3 final {
    float x;
    float y;
    float z;

    // 定数
    static const Vector3 zero;
    static const Vector3 one;
    static const Vector3 right;
    static const Vector3 up;
    static const Vector3 forward;

    /**
     * @brief 添え字演算子
     * @param index 成分のインデックス (0:x, 1:y, 2:z)
     * @return 成分への参照
     */
    float& operator[](int index);

    /**
     * @brief 添え字演算子 (const)
     * @param index 成分のインデックス (0:x, 1:y, 2:z)
     * @return 成分の値
     */
    float operator[](int index) const;

    /** @name 複合代入演算子 */
    /** @{ */
    Vector3& operator+=(const Vector3& rhs);
    Vector3& operator-=(const Vector3& rhs);
    Vector3& operator*=(float s);
    Vector3& operator/=(float s);
    Vector3& operator*=(const Vector3& rhs);
    Vector3& operator/=(const Vector3& rhs);
    /** @} */

    /** @name 比較演算子 */
    /** @{ */
    bool operator==(const Vector3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    bool operator!=(const Vector3& rhs) const {
        return !(*this == rhs);
    }
    /**
     * @brief Equals を実行する。
     */
    bool Equals(const Vector3& other, float epsilon = 1e-5f) const {
        return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon && std::abs(z - other.z) <= epsilon;
    }
    /** @} */

    /** @name 数学関数 */
    /** @{ */
    inline float LengthSquared() const {
        return x * x + y * y + z * z;
    }
    /**
     * @brief LengthSq を実行する。
     */
    inline float LengthSq() const {
        return LengthSquared();
    }
    /**
     * @brief Length を実行する。
     */
    inline float Length() const {
        return std::sqrt(LengthSquared());
    }
    /**
     * @brief Normalize を実行する。
     */
    inline void Normalize() {
        float lenSq = LengthSquared();
        if (lenSq > 0.0f) {
            float invLen = 1.0f / std::sqrt(lenSq);
            x *= invLen;
            y *= invLen;
            z *= invLen;
        }
    }
    /**
     * @brief Normalized を取得する。
     * @return 取得された Normalized
     */
    inline Vector3 GetNormalized() const {
        Vector3 v = *this;
        v.Normalize();
        return v;
    }
    /**
     * @brief Dot を実行する。
     */
    inline float Dot(const Vector3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    /**
     * @brief Cross を実行する。
     */
    inline Vector3 Cross(const Vector3& rhs) const {
        return {y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x};
    }
    /** @} */

    /** @name データアクセサ */
    /** @{ */
    const float* data() const {
        return &x;
    }
    /**
     * @brief data を実行する。
     */
    float* data() {
        return &x;
    }
    /** @} */
};

/** @name 非メンバ演算子 */
/** @{ */

Vector3 operator+(const Vector3& lhs, const Vector3& rhs);
Vector3 operator-(const Vector3& lhs, const Vector3& rhs);
Vector3 operator+(const Vector3& v);
Vector3 operator-(const Vector3& v);
Vector3 operator*(const Vector3& v, float s);
Vector3 operator*(float s, const Vector3& v);
Vector3 operator/(const Vector3& v, float s);

// 要素ごとの乗除算
Vector3 operator*(const Vector3& lhs, const Vector3& rhs);
Vector3 operator/(const Vector3& lhs, const Vector3& rhs);

/** @} */

} // namespace Irufemi
