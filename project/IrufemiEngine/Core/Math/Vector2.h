#pragma once

#include <cmath>
#include <cassert>

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
    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 2);
        return (&x)[index];
    }

    /**
     * @brief 添え字演算子 (const)
     * @param index 成分のインデックス (0:x, 1:y)
     * @return 成分の値
     */
    float operator[](int index) const noexcept {
        assert(index >= 0 && index < 2);
        return (&x)[index];
    }

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Vector2& operator+=(const Vector2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    constexpr Vector2& operator-=(const Vector2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    constexpr Vector2& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        return *this;
    }
    Vector2& operator/=(float s) noexcept {
        assert(s != 0.0f);
        const float inv = 1.0f / s;
        x *= inv;
        y *= inv;
        return *this;
    }
    constexpr Vector2& operator*=(const Vector2& rhs) noexcept {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }
    Vector2& operator/=(const Vector2& rhs) noexcept {
        assert(rhs.x != 0.0f && rhs.y != 0.0f);
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }
    /** @} */

    /** @name 比較演算子 */
    /** @{ */
    bool operator==(const Vector2& rhs) const {
        return x == rhs.x && y == rhs.y;
    }
    bool operator!=(const Vector2& rhs) const {
        return !(*this == rhs);
    }
    /**
     * @brief Equals を実行する。
     */
    bool Equals(const Vector2& other, float epsilon = 1e-5f) const {
        return std::abs(x - other.x) <= epsilon && std::abs(y - other.y) <= epsilon;
    }
    /** @} */

    /** @name 数学関数 */
    /** @{ */
    inline float LengthSquared() const {
        return x * x + y * y;
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
    inline float Dot(const Vector2& rhs) const {
        return x * rhs.x + y * rhs.y;
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
inline constexpr Vector2 Vector2::zero{0.0f, 0.0f};
inline constexpr Vector2 Vector2::one{1.0f, 1.0f};
inline constexpr Vector2 Vector2::right{1.0f, 0.0f};
inline constexpr Vector2 Vector2::up{0.0f, 1.0f};

/** @name 非メンバ演算子 */
/** @{ */

[[nodiscard]] constexpr inline Vector2 operator+(const Vector2& lhs, const Vector2& rhs) noexcept {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}
[[nodiscard]] constexpr inline Vector2 operator-(const Vector2& lhs, const Vector2& rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}
[[nodiscard]] constexpr inline Vector2 operator+(const Vector2& v) noexcept {
    return v;
}
[[nodiscard]] constexpr inline Vector2 operator-(const Vector2& v) noexcept {
    return {-v.x, -v.y};
}
[[nodiscard]] constexpr inline Vector2 operator*(const Vector2& v, float s) noexcept {
    return {v.x * s, v.y * s};
}
[[nodiscard]] constexpr inline Vector2 operator*(float s, const Vector2& v) noexcept {
    return {v.x * s, v.y * s};
}
[[nodiscard]] inline Vector2 operator/(const Vector2& v, float s) noexcept {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return {v.x * inv, v.y * inv};
}

// 要素ごとの乗除算
[[nodiscard]] constexpr inline Vector2 operator*(const Vector2& lhs, const Vector2& rhs) noexcept {
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}
[[nodiscard]] inline Vector2 operator/(const Vector2& lhs, const Vector2& rhs) noexcept {
    assert(rhs.x != 0.0f && rhs.y != 0.0f);
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

/** @} */

} // namespace Irufemi
