#pragma once

#include <vector>

namespace Irufemi {

/**
 * @class PerlinNoise
 * @brief 1D/2D/3Dの滑らかなノイズ（パーリンノイズ）を生成するユーティリティクラス
 */
class PerlinNoise {
public:
    // シード値を指定して初期化
    PerlinNoise(unsigned int seed = 0);

    // 1D ノイズ (0.0 ~ 1.0)
    float Noise(float x) const;
    // 2D ノイズ (0.0 ~ 1.0)
    float Noise(float x, float y) const;
    // 3D ノイズ (0.0 ~ 1.0)
    float Noise(float x, float y, float z) const;

    // -1.0 ~ 1.0 の範囲で返すユーティリティ
    float Noise1D(float x) const;
    float Noise2D(float x, float y) const;
    float Noise3D(float x, float y, float z) const;

private:
    std::vector<int> p;

    float Fade(float t) const;
    float Lerp(float t, float a, float b) const;
    float Grad(int hash, float x, float y, float z) const;
};

} // namespace Irufemi
