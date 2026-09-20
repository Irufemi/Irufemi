#pragma once

#include <cmath>
#include <cassert>

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
    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 4);
        return (&x)[index];
    }

    /**
     * @brief 添え字演算子 (const)
     * @param index 成分のインデックス (0:x, 1:y, 2:z, 3:w)
     * @return 成分の値
     */
    float operator[](int index) const noexcept {
        assert(index >= 0 && index < 4);
        return (&x)[index];
    }

    /** @name 単項演算子 */
    /** @{ */
    constexpr Vector4 operator+() const noexcept {
        return *this;
    }
    constexpr Vector4 operator-() const noexcept {
        return {-x, -y, -z, -w};
    }
    /** @} */

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Vector4& operator+=(const Vector4& v) noexcept {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }
    constexpr Vector4& operator-=(const Vector4& v) noexcept {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
        return *this;
    }
    constexpr Vector4& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    Vector4& operator/=(float s) noexcept {
        assert(s != 0.0f);
        const float inv = 1.0f / s;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }
    constexpr Vector4& operator*=(const Vector4& rhs) noexcept {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }
    Vector4& operator/=(const Vector4& rhs) noexcept {
        assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }
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

// 定数定義 (C++17 インライン変数)
inline constexpr Vector4 Vector4::zero{0.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 Vector4::one{1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr Vector4 Vector4::right{1.0f, 0.0f, 0.0f, 0.0f};
inline constexpr Vector4 Vector4::up{0.0f, 1.0f, 0.0f, 0.0f};
inline constexpr Vector4 Vector4::forward{0.0f, 0.0f, 1.0f, 0.0f};

/** @name 非メンバ演算子 */
/** @{ */

[[nodiscard]] constexpr inline Vector4 operator+(const Vector4& v1, const Vector4& v2) noexcept {
    return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}
[[nodiscard]] constexpr inline Vector4 operator-(const Vector4& v1, const Vector4& v2) noexcept {
    return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}
[[nodiscard]] constexpr inline Vector4 operator*(const Vector4& v, float s) noexcept {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
}
[[nodiscard]] constexpr inline Vector4 operator*(float s, const Vector4& v) noexcept {
    return {v.x * s, v.y * s, v.z * s, v.w * s};
}
[[nodiscard]] inline Vector4 operator/(const Vector4& v, float s) noexcept {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return {v.x * inv, v.y * inv, v.z * inv, v.w * inv};
}

// 要素ごとの乗除算
[[nodiscard]] constexpr inline Vector4 operator*(const Vector4& lhs, const Vector4& rhs) noexcept {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}
[[nodiscard]] inline Vector4 operator/(const Vector4& lhs, const Vector4& rhs) noexcept {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f && rhs.w != 0.0f);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

/** @} */

} // namespace Irufemi
