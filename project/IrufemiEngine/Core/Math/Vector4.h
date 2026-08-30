#pragma once

#include <cmath>

namespace Irufemi {
/**
 * @struct Vector4
 * @brief 4次元ベクトル
 */
struct Vector4 final {
    float x;
    float y;
    float z;
    float w;

    // 定数
    static const Vector4 zero;
    static const Vector4 one;
    static const Vector4 right;
    static const Vector4 up;
    static const Vector4 forward;

    /**
     * @brief 添え字演算子
     * @param index 成分のインデックス (0:x, 1:y, 2:z, 3:w)
     * @return 成分への参照
     */
    float& operator[](int index);

    /**
     * @brief 添え字演算子 (const)
     * @param index 成分のインデックス (0:x, 1:y, 2:z, 3:w)
     * @return 成分の値
     */
    float operator[](int index) const;

    /** @name 単項演算子 */
    /** @{ */
    Vector4 operator+() const;
    Vector4 operator-() const;
    /** @} */

    /** @name 複合代入演算子 */
    /** @{ */
    Vector4& operator+=(const Vector4& v);
    Vector4& operator-=(const Vector4& v);
    Vector4& operator*=(float s);
    Vector4& operator/=(float s);
    Vector4& operator*=(const Vector4& rhs);
    Vector4& operator/=(const Vector4& rhs);
    /** @} */

    /** @name 比較演算子 */
    /** @{ */
    bool operator==(const Vector4& v) const {
        return x == v.x && y == v.y && z == v.z && w == v.w;
    }
    bool operator!=(const Vector4& v) const {
        return !(*this == v);
    }
    /**
     * @brief Equals を実行する。
     */
    bool Equals(const Vector4& other, float epsilon = 1e-5f) const {
        return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon &&
               std::abs(z - other.z) <= epsilon && std::abs(w - other.w) <= epsilon;
    }
    /** @} */

    /** @name 数学関数 */
    /** @{ */
    inline float LengthSquared() const {
        return x * x + y * y + z * z + w * w;
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
            w *= invLen;
        }
    }
    /**
     * @brief Normalized を取得する。
     * @return 取得された Normalized
     */
    inline Vector4 GetNormalized() const {
        Vector4 v = *this;
        v.Normalize();
        return v;
    }
    /**
     * @brief Dot を実行する。
     */
    inline float Dot(const Vector4& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
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

Vector4 operator+(const Vector4& v1, const Vector4& v2);
Vector4 operator-(const Vector4& v1, const Vector4& v2);
Vector4 operator*(const Vector4& v, float s);
Vector4 operator*(float s, const Vector4& v);
Vector4 operator/(const Vector4& v, float s);

// 要素ごとの乗除算
Vector4 operator*(const Vector4& lhs, const Vector4& rhs);
Vector4 operator/(const Vector4& lhs, const Vector4& rhs);

/** @} */

} // namespace Irufemi
