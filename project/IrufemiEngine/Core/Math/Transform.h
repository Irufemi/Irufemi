#pragma once

#include "Core/Math/Vector3.h"

namespace Irufemi {
/**
 * @class Transform
 * @brief 3D空間における位置・回転・スケールの状態を保持する構造体
 * @details GameObjectの座標変換の基礎となり、Matrix4x4の構築や親子関係の計算に使用されます。
 */
struct Transform {
    /** @brief ローカル空間での各軸の拡大縮小率 (デフォルト: 1.0) */
    Vector3 scale{1.0f, 1.0f, 1.0f};

    /** @brief ローカル空間での各軸の回転角度（オイラー角・ラジアン） */
    Vector3 rotate{0.0f, 0.0f, 0.0f};

    /** @brief ローカル空間での平行移動量 (位置) */
    Vector3 translate{0.0f, 0.0f, 0.0f};
};
} // namespace Irufemi
