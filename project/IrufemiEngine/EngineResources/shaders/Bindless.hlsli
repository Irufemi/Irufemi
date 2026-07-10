#pragma once

/**
 * @file Bindless.hlsli
 * @brief Bindless Resources 用のテクスチャ配列定義
 */

// Bindless 用のテクスチャ配列 (Space1, Slot 2 にマッピング)
Texture2D<float32_t4> gTextures[] : register(t0, space1);

// Bindless 用の環境マップ配列 (Space2 にマッピング)
TextureCube<float32_t4> gTextureCubes[] : register(t0, space2);
