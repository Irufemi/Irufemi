#pragma once

namespace Irufemi {
/**
 * @struct Matrix4x4
 * @brief 4x4 行列
 */
struct Matrix4x4 final {
    float m[4][4];

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Matrix4x4& operator+=(const Matrix4x4& rhs) noexcept {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m[i][j] += rhs.m[i][j];
            }
        }
        return *this;
    }
    constexpr Matrix4x4& operator-=(const Matrix4x4& rhs) noexcept {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m[i][j] -= rhs.m[i][j];
            }
        }
        return *this;
    }
    constexpr Matrix4x4& operator*=(const Matrix4x4& rhs) noexcept {
        Matrix4x4 result{};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 4; ++k) {
                    result.m[i][j] += m[i][k] * rhs.m[k][j];
                }
            }
        }
        *this = result;
        return *this;
    }
    /** @} */
};

/** @name 非メンバ演算子 */
/** @{ */

/**
 * @brief 行列の加算
 */
[[nodiscard]] constexpr inline Matrix4x4 operator+(const Matrix4x4& lhs, const Matrix4x4& rhs) noexcept {
    Matrix4x4 result = lhs;
    return result += rhs;
}

/**
 * @brief 行列の減算
 */
[[nodiscard]] constexpr inline Matrix4x4 operator-(const Matrix4x4& lhs, const Matrix4x4& rhs) noexcept {
    Matrix4x4 result = lhs;
    return result -= rhs;
}

/**
 * @brief 行列の積 (lhs * rhs)
 */
[[nodiscard]] constexpr inline Matrix4x4 operator*(const Matrix4x4& lhs, const Matrix4x4& rhs) noexcept {
    Matrix4x4 result = lhs;
    return result *= rhs;
}

/**
 * @brief 単項演算子 +
 */
[[nodiscard]] constexpr inline Matrix4x4 operator+(const Matrix4x4& m) noexcept {
    return m;
}

/**
 * @brief 単項演算子 - (符号反転)
 */
[[nodiscard]] constexpr inline Matrix4x4 operator-(const Matrix4x4& m) noexcept {
    Matrix4x4 result{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = -m.m[i][j];
        }
    }
    return result;
}

/** @} */

} // namespace Irufemi
