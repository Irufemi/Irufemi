#pragma once

/**
 * @file Material.hlsli
 * @brief 統一マテリアル構造体の定義
 */

struct Material {
    float32_t4 color;           //!< ベースカラー
    int32_t enableLighting;      //!< ライティング有効フラグ
    int32_t hasTexture;          //!< テクスチャ有効フラグ
    int32_t lightingMode;        //!< 0:None, 1:Lambert, 2:Half-Lambert
    float32_t padding;           //!< パディング (16byteアラインメント用)
    
    float32_t4x4 uvTransform;    //!< UV座標変換行列
    
    float32_t shininess;         //!< 光沢度 (Blinn-Phong用)
    int32_t useClampSampler;     //!< パーティクル等で使用するサンプラー切替 (0:WRAP, 1:CLAMP)
    float32_t2 padding2;         //!< 最終的なアラインメント調整
};
