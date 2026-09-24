#pragma once
#include "Core/Math/Matrix4x4.h"

namespace Irufemi {
/**
 * @brief ビュー（カメラ）ごとの定数バッファデータ構造体 (HLSL連動)
 */
struct PerView {
    Matrix4x4 viewProjection;  ///< ビュープロジェクション行列
    Matrix4x4 billboardMatrix; ///< ビルボード用回転行列
    Vector3 worldPosition;     ///< カメラのワールド座標
    float cameraNear;          ///< カメラのNearクリップ距離
    float cameraFar;           ///< カメラのFarクリップ距離
    float pad[3];              ///< 16バイトアライメント用パディング
};
} // namespace Irufemi
