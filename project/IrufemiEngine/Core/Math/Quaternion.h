#pragma once

#include <cassert>

namespace Irufemi {
/**
 * @struct Quaternion
 * @brief クォータニオン (四元数)
 */
struct Quaternion final {
    float x;
    float y;
    float z;
    float w;

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

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Quaternion& operator+=(const Quaternion& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }
    constexpr Quaternion& operator-=(const Quaternion& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }
    constexpr Quaternion& operator*=(float s) noexcept {
        x *= s;
        y *= s;
        z *= s;
        w *= s;
        return *this;
    }
    Quaternion& operator/=(float s) noexcept {
        assert(s != 0.0f);
        const float inv = 1.0f / s;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }
    /** @} */
};

/** @name 非メンバ演算子 */
/** @{ */

/**
 * @brief クォータニオン同士の加算
 */
[[nodiscard]] constexpr inline Quaternion operator+(const Quaternion& lhs, const Quaternion& rhs) noexcept {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

/**
 * @brief クォータニオン同士の減算
 */
[[nodiscard]] constexpr inline Quaternion operator-(const Quaternion& lhs, const Quaternion& rhs) noexcept {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

/**
 * @brief 単項演算子 +
 */
[[nodiscard]] constexpr inline Quaternion operator+(const Quaternion& q) noexcept {
    return q;
}

/**
 * @brief 単項演算子 - (符号反転)
 */
[[nodiscard]] constexpr inline Quaternion operator-(const Quaternion& q) noexcept {
    return {-q.x, -q.y, -q.z, -q.w};
}

/**
 * @brief スカラー乗算
 */
[[nodiscard]] constexpr inline Quaternion operator*(const Quaternion& q, float s) noexcept {
    return {q.x * s, q.y * s, q.z * s, q.w * s};
}

/**
 * @brief スカラー乗算 (可換)
 */
[[nodiscard]] constexpr inline Quaternion operator*(float s, const Quaternion& q) noexcept {
    return {q.x * s, q.y * s, q.z * s, q.w * s};
}

/**
 * @brief スカラー除算
 */
[[nodiscard]] inline Quaternion operator/(const Quaternion& q, float s) noexcept {
    assert(s != 0.0f);
    const float inv = 1.0f / s;
    return {q.x * inv, q.y * inv, q.z * inv, q.w * inv};
}

/**
 * @brief クォータニオン同士の積 (ハミルトン積)
 */
[[nodiscard]] constexpr inline Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs) noexcept {
    return {lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x,
            lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w,
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z};
}

/** @} */

} // namespace Irufemi
