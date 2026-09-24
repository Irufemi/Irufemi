#pragma once

namespace Irufemi {
/**
 * @struct Vector3Int
 * @brief 3次元整数ベクトル（グリッド座標・解像度用）
 */
struct Vector3Int {
    int x = 0;
    int y = 0;
    int z = 0;

    constexpr bool operator==(const Vector3Int& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }

    constexpr bool operator!=(const Vector3Int& other) const noexcept {
        return !(*this == other);
    }

    constexpr Vector3Int& operator+=(const Vector3Int& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vector3Int& operator-=(const Vector3Int& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
};

[[nodiscard]] constexpr inline Vector3Int operator+(const Vector3Int& lhs, const Vector3Int& rhs) noexcept {
    Vector3Int result = lhs;
    return result += rhs;
}

[[nodiscard]] constexpr inline Vector3Int operator-(const Vector3Int& lhs, const Vector3Int& rhs) noexcept {
    Vector3Int result = lhs;
    return result -= rhs;
}
} // namespace Irufemi
