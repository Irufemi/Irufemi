#pragma once

#include "Core/Math/Vector4.h"

namespace Irufemi {

/**
 * @struct LineVertexData
 * @brief 単純なライン描画用の頂点データ構造体
 */
struct LineVertexData {
    Vector4 position{}; //!< 頂点座標 (XYZW)
    Vector4 color{};    //!< 頂点カラー (RGBA)
};

} // namespace Irufemi