#pragma once
#include "../../Core/Math/Vector4.h"

/**
 * @struct LightningParams
 * @brief 電撃エフェクト（Lightning Crawl）の調整用パラメータ構造体
 */
struct LightningParams {
    Vector4 color = { 0.8f, 0.4f, 1.0f, 1.0f }; //!< 電撃の色 (HDR対応)
    float speed = 1.0f;                         //!< アニメーション速度
    float intensity = 1.0f;                     //!< 輝きの強さ (Bloom影響)
    float noiseScale = 1.0f;                    //!< ノイズの細かさ
    float noiseThreshold = 0.5f;                //!< 雷の出現しきい値 (低いほど太くなる)
};
