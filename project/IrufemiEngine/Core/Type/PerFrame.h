#pragma once

namespace Irufemi {
/**
 * @struct PerFrame
 * @brief フレーム単位の時間データ（Compute / ConstantBuffer 用）
 * @details Direct3D 12 の 16バイト境界（float4境界）アライメントを満たすためパディングを含みます。
 */
struct PerFrame {
    float time = 0.0f;
    float deltaTime = 0.0f;
    float pad_[2] = {0.0f, 0.0f}; ///< 16バイト境界アライメント用パディング
};
} // namespace Irufemi
