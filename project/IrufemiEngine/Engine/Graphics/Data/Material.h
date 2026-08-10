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
    Irufemi::Vector4 color;              //!< ベースカラー
    int32_t enableLighting;      //!< ライティング有効フラグ
    int32_t hasTexture;          //!< テクスチャ有効フラグ
    int32_t lightingMode;        //!< 0:None, 1:Lambert, 2:Half-Lambert, 3:PBR
    float environmentCoefficient; //!< 環境マップの映り込み係数
    
    Irufemi::Matrix4x4 uvTransform;       //!< UV座標変換行列
    
    float metallic;              //!< 金属度
    float roughness;             //!< 粗さ
    int32_t useClampSampler;     //!< パーティクル等で使用するサンプラー切替 (0:WRAP, 1:CLAMP)
    float alphaReference;        //!< ディスカード閾値 (0.0f = 全部描画, 1.0f = 全部棄却)
    
    uint32_t textureIndex;       //!< [Bindless] メインテクスチャのSRVインデックス (space1)
    uint32_t envMapIndex;        //!< [Bindless] 環境マップのSRVインデックス (space2)
    int32_t customEffectType;    //!< カスタムエフェクトのタイプ (0: なし)
    float customEffectParam;     //!< カスタムエフェクトのパラメータ
    
    int32_t enableEffectMask;    //!< 1: エフェクト等のマスクバッファにシルエットを出力する
    int32_t padding[3];          //!< 16バイトアライメント用
};
