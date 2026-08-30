#pragma once

#include <cstdint>
#include "Core/Math/Vector3.h"


namespace Irufemi {
/**
 * @class Sphere
 * @brief 球体を表す幾何学構造体
 * @details 中心座標と半径を持ち、最も計算コストが低い衝突判定や、影響範囲の定義に使用されます。
 */
struct Sphere {
    /** @brief 球の中心座標 */
	Vector3 center = {0.0f, 0.0f, 0.0f};

    /** @brief 球の半径 */
    float radius = 1.0f;
};


} // namespace Irufemi
