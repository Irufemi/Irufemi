#include "PostProcessParameters.hlsli"
#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"

/**
 * @file GaussianBlur.PS.hlsl
 * @brief 分離型ガウスぼかしシェーダー（縦または横の1次元パス）
 */



ConstantBuffer<BloomParams> gBloom : register(b0);
SamplerState gSampler : register(s0);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    int32_t halfSize = gBloom.kernelSize / 2;
    output.color.rgb = ApplyGaussian1D_Optimized(gTexture, gSampler, input.texcoord, gBloom.direction, gBloom.sigma, halfSize);
    output.color.a = 1.0f;
    
    return output;
}
