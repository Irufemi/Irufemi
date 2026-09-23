#pragma once

#include "Core/Math/Vector3.h"
#include "Core/Shape/Ball.h"

namespace Irufemi {

/**
 * @struct Spring
 * @brief バネ物理シミュレーション用のデータ構造体
 */
struct Spring {
    Vector3 anchor{};            //!< アンカー（固定端）の位置
    float naturalLength = 0.0f;  //!< 自然長
    float stiffness = 0.0f;      //!< 剛性（バネ定数 k）
    float dampingCoefficient = 0.0f; //!< 減衰係数
    Ball ball{};                 //!< 接続されているボール
};

} // namespace Irufemi
