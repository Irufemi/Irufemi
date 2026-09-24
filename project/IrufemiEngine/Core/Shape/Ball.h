#pragma once

#include "Core/Math/Vector3.h"

namespace Irufemi {
/**
 * @brief 物理シミュレーション用の球体構造体
 */
struct Ball {
    Vector3 position{};              ///< ボールの位置
    Vector3 velocity{};              ///< ボールの速度
    Vector3 acceleration{};          ///< ボールの加速度
    float mass = 1.0f;               ///< ボールの質量
    float radius = 1.0f;             ///< ボールの半径
    unsigned int color = 0xFFFFFFFF; ///< ボールの色 (RGBA/HEX)
};
} // namespace Irufemi
