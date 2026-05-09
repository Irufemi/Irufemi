#include "Fullscreen.hlsli"

/**
 * @file GaussianBlur.PS.hlsl
 * @brief 分離型ガウスぼかしシェーダー（縦または横の1次元パス）
 */

struct BloomParams {
    float32_t2 direction;   // ぼかし方向 ({1,0} or {0,1})
    float32_t threshold;
    float32_t sigma;
    float32_t intensity;
    int32_t kernelSize;
};

ConstantBuffer<BloomParams> gBloom : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t PI = 3.14159265f;

// 1次元ガウス関数
float32_t gauss(float32_t x, float32_t sigma) {
    float32_t exponent = -(x * x) * rcp(2.0f * sigma * sigma);
    float32_t denominator = sqrt(2.0f * PI) * sigma;
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
    
    int32_t halfSize = gBloom.kernelSize / 2;
    
    // 指定された方向に沿って1次元の畳み込みを行う
    for (int32_t i = -halfSize; i <= halfSize; ++i) {
        float32_t w = gauss(float32_t(i), gBloom.sigma);
        
        float32_t2 offset = gBloom.direction * float32_t(i) * texelSize;
        sum += gTexture.Sample(gSampler, uv + offset).rgb * w;
        weightTotal += w;
    }
    
    output.color.rgb = sum * rcp(weightTotal);
    output.color.a = 1.0f;
    
    return output;
}
