#pragma once

#include "../Vector3.h"


namespace Irufemi {
/**
 * @class OBB
 * @brief Oriented Bounding Box (有向境界箱) を表す構造体
 * @details 任意の回転を持つ直方体の境界を定義し、より精密な衝突判定（ナローフェーズ）に使用されます。
 */
struct OBB {
    /** @brief ボックスの中心座標 */
    Vector3 center;

    /** @brief ローカルの各軸方向を表す正規化された3つの基底ベクトル（直交必須） */
    Vector3 orientations[3];

    /** @brief 中心から各面までの距離（各軸の長さの半分 / Extents） */
    Vector3 size;
};
} // namespace Irufemi
