#pragma once

#include "../Math/Vector3.h"


namespace Irufemi {
//平面とは無限遠平面のこと。範囲に限りがない。

/**
 * @class Plane
 * @brief 3D空間上の無限平面を表す構造体
 * @details 法線ベクトルと原点からの距離を持ち、フラスタムカリングや空間分割などに使用されます。
 */
struct Plane{
    /** @brief 平面の向きを表す正規化された法線ベクトル */
	Vector3 normal{};

    /** @brief 原点から法線方向に離れている距離 (通常は dot(normal, point) = distance) */
    float distance{};
};


} // namespace Irufemi
