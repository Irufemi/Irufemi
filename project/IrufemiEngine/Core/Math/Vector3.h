#pragma once

#include <cmath>
#include <cassert>

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
    float& operator[](int index) noexcept {
        assert(index >= 0 && index < 3);
        return (&x)[index];
    }

    /**
     * @brief 添え字演算子 (const)
     * @param index 成分のインデックス (0:x, 1:y, 2:z)
     * @return 成分の値
     */
    float operator[](int index) const noexcept {
        assert(index >= 0 && index < 3);
        return (&x)[index];
    }

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Vector3& operator+=(const Vector3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    constexpr Vector3& operator-=(const Vector3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
    constexpr Vector3& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    Vector3& operator/=(float s) noexcept {
        assert(s != 0.0f);
        const float inv = 1.0f / s;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }
    constexpr Vector3& operator*=(const Vector3& rhs) noexcept {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }
    Vector3& operator/=(const Vector3& rhs) noexcept {
        assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        return *this;
    }
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

// 定数定義 (C++17 インライン変数)
inline constexpr Vector3 Vector3::zero{0.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::one{1.0f, 1.0f, 1.0f};
inline constexpr Vector3 Vector3::right{1.0f, 0.0f, 0.0f};
inline constexpr Vector3 Vector3::up{0.0f, 1.0f, 0.0f};
inline constexpr Vector3 Vector3::forward{0.0f, 0.0f, 1.0f};

/** @name 非メンバ演算子 */
/** @{ */

[[nodiscard]] constexpr inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs) noexcept {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}
[[nodiscard]] constexpr inline Vector3 operator-(const Vector3& lhs, const Vector3& rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}
[[nodiscard]] constexpr inline Vector3 operator+(const Vector3& v) noexcept {
    return v;
}
[[nodiscard]] constexpr inline Vector3 operator-(const Vector3& v) noexcept {
    return {-v.x, -v.y, -v.z};
}
[[nodiscard]] constexpr inline Vector3 operator*(const Vector3& v, float s) noexcept {
    return {v.x * s, v.y * s, v.z * s};
}
[[nodiscard]] constexpr inline Vector3 operator*(float s, const Vector3& v) noexcept {
    return {v.x * s, v.y * s, v.z * s};
}
[[nodiscard]] inline Vector3 operator/(const Vector3& v, float s) noexcept {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return {v.x * inv, v.y * inv, v.z * inv};
}

// 要素ごとの乗除算
[[nodiscard]] constexpr inline Vector3 operator*(const Vector3& lhs, const Vector3& rhs) noexcept {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}
[[nodiscard]] inline Vector3 operator/(const Vector3& lhs, const Vector3& rhs) noexcept {
    assert(rhs.x != 0.0f && rhs.y != 0.0f && rhs.z != 0.0f);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

/** @} */

} // namespace Irufemi
