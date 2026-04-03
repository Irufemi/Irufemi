#pragma once

#include "../../Core/Math/Vector4.h"
#include "../../Core/Math/Matrix4x4.h"
#include <cstdint>

/**
 * @struct Material
 * @brief 統一マテリアル構造体
 * HLSL側の Material とメモリレイアウトを完全に一致させる
 */
struct Material {
    Vector4 color;              //!< ベースカラー
    int32_t enableLighting;      //!< ライティング有効フラグ
    int32_t hasTexture;          //!< テクスチャ有効フラグ
    int32_t lightingMode;        //!< 0:None, 1:Lambert, 2:Half-Lambert
    float padding;               //!< パディング (16byteアラインメント用)
    
    Matrix4x4 uvTransform;       //!< UV座標変換行列
    
    float shininess;             //!< 光沢度 (Blinn-Phong用)
    int32_t useClampSampler;     //!< サンプラー切替 (0:WRAP, 1:CLAMP)
    float padding2[2];           //!< パディング
};
