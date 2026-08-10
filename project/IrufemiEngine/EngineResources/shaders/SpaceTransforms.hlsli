#pragma once

/**
 * @file SpaceTransforms.hlsli
 * @brief 空間変換（座標変換）に特化したユーティリティ関数群
 */

// NDC深度値からView空間のZ値を復元する
float32_t ReconstructViewZ(float32_t ndcDepth, float32_t4x4 projInverse)
{
    float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), projInverse);
    return viewSpace.z * rcp(viewSpace.w);
}

// UVとNDC深度値からView空間座標を復元する
float32_t3 ReconstructViewPosition(float32_t2 uv, float32_t ndcDepth, float32_t4x4 projInverse)
{
    float32_t x = uv.x * 2.0f - 1.0f;
    float32_t y = -uv.y * 2.0f + 1.0f;
    float32_t4 viewSpace = mul(float32_t4(x, y, ndcDepth, 1.0f), projInverse);
    return viewSpace.xyz * rcp(viewSpace.w);
}
