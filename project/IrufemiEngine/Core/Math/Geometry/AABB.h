#pragma once

#include <cstdint>
#include "Core/Math/Vector3.h"


namespace Irufemi {
//AABB(Axis Sligned Bounding Box)
/**
 * @class AABB
 * @brief Axis-Aligned Bounding Box (軸平行境界箱) を表す構造体
 * @details 回転を持たない直方体の境界を定義し、高速な大まかな衝突判定（ブロードフェーズ）などに使用されます。
 */
struct AABB {
    /** @brief 境界箱の最小座標（各軸の最小値） */
	Vector3 min{-1.0f, -1.0f, -1.0f};

    /** @brief 境界箱の最大座標（各軸の最大値） */
	Vector3 max{1.0f, 1.0f, 1.0f};
};
} // namespace Irufemi
