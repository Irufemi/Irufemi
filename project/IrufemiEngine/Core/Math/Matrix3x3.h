#pragma once

namespace Irufemi {
/**
 * @struct Matrix3x3
 * @brief 3x3 行列
 */
struct Matrix3x3 final {
    float m[3][3];

    /**
     * @brief 単位行列を取得する
     */
    [[nodiscard]] static constexpr Matrix3x3 Identity() noexcept {
        return Matrix3x3{
            {{1.0f, 0.0f, 0.0f},
             {0.0f, 1.0f, 0.0f},
             {0.0f, 0.0f, 1.0f}}
        };
    }

    /** @name 複合代入演算子 */
    /** @{ */
    constexpr Matrix3x3& operator+=(const Matrix3x3& rhs) noexcept {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m[i][j] += rhs.m[i][j];
            }
        }
        return *this;
    }

    constexpr Matrix3x3& operator-=(const Matrix3x3& rhs) noexcept {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                m[i][j] -= rhs.m[i][j];
            }
        }
        return *this;
    }

    constexpr Matrix3x3& operator*=(const Matrix3x3& rhs) noexcept {
        Matrix3x3 result{};
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
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

[[nodiscard]] constexpr inline Matrix3x3 operator+(const Matrix3x3& lhs, const Matrix3x3& rhs) noexcept {
    Matrix3x3 result = lhs;
    return result += rhs;
}

[[nodiscard]] constexpr inline Matrix3x3 operator-(const Matrix3x3& lhs, const Matrix3x3& rhs) noexcept {
    Matrix3x3 result = lhs;
    return result -= rhs;
}

[[nodiscard]] constexpr inline Matrix3x3 operator*(const Matrix3x3& lhs, const Matrix3x3& rhs) noexcept {
    Matrix3x3 result = lhs;
    return result *= rhs;
}

/** @} */
} // namespace Irufemi
