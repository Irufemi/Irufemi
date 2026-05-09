#include "Fullscreen.hlsli"

// ガウスフィルタ用定数バッファ
struct GaussianParams {
    float32_t sigma;
    int32_t kernelSize;
};

ConstantBuffer<GaussianParams> gParams : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t PI = 3.14159265f;

// 2次元ガウス関数
float32_t gauss(float32_t x, float32_t y, float32_t sigma) {
    float32_t exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float32_t denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    float32_t2 uv = input.texcoord;
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 texelSize = rcp(float32_t2(width, height));
    
    float32_t3 sum = 0.0f;
    float32_t weightTotal = 0.0f;
    
    // カーネルサイズに基づいて畳み込み（中心を 0,0 とした相対座標）
    int32_t halfSize = gParams.kernelSize / 2;
    
    for (int32_t x = -halfSize; x <= halfSize; ++x) {
        for (int32_t y = -halfSize; y <= halfSize; ++y) {
            // ガウス重みを計算
            float32_t w = gauss(float32_t(x), float32_t(y), gParams.sigma);
            
            // サンプリング座標
            float32_t2 offset = float32_t2(x, y) * texelSize;
            float32_t3 sample = gTexture.Sample(gSampler, uv + offset).rgb;
            
            sum += sample * w;
            weightTotal += w;
        }
    }
    
    // 重みの合計で割って正規化（エネルギー保存）
    output.color.rgb = sum * rcp(weightTotal);
    output.color.a = 1.0f;
    
    return output;
}
