#pragma once

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

/**
 * @struct VertexData
 * @brief 3Dモデルなどの頂点データを表す構造体
 */
struct VertexData {
    Irufemi::Vector4 position;
    Irufemi::Vector2 texcoord;
    Irufemi::Vector3 normal;
    Irufemi::Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f}; ///< [追加] 頂点カラー
};
