#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

/**
 * @file BloomCombine.PS.hlsl
 * @brief オリジナル画像とボケた高輝度画像を合成するシェーダー
 */

struct BloomParams {
    float32_t2 direction;
    float32_t threshold;
    float32_t sigma;
    float32_t intensity;
    int32_t kernelSize;
};

ConstantBuffer<BloomParams> gBloom : register(b0);

SamplerState gSampler : register(s0);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    float32_t4 original = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 blurred = gExtraTexture.Sample(gSampler, input.texcoord);
    
    // 加算合成 (Intensity を掛けて強調)
    float32_t3 finalRGB = original.rgb + blurred.rgb * gBloom.intensity;
    
    output.color = float32_t4(finalRGB, original.a);
    
    return output;
}
