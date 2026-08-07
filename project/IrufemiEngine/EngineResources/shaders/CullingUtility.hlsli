#pragma once

/**
 * @file CullingUtility.hlsli
 * @brief ハードウェアのラスタライザを利用して、ピクセルシェーダーの起動をスキップするためのカリング関数群
 */

/**
 * @brief 面積を0にして縮退ポリゴン（Degenerate Triangle）としてハードウェアに破棄させる
 * @param scale パーティクル等のスケール値
 */
inline void CullInstanceByScale(inout float3 scale)
{
    scale = float3(0.0f, 0.0f, 0.0f);
}

/**
 * @brief 頂点座標をフラスタム外（画面外）に飛ばしてクリッピングさせる
 * @param clipPosition SV_POSITION 等のクリップ空間座標
 */
inline void CullVertexByPosition(inout float4 clipPosition)
{
    // Y座標を遥か下方に飛ばすことで、フラスタムカリングの対象とする
    clipPosition = float4(0.0f, -10000.0f, 0.0f, 1.0f);
}
