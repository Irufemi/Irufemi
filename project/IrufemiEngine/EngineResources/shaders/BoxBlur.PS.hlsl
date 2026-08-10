#include "PostProcessParameters.hlsli"
#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"

/**
 * @file BoxBlur.PS.hlsl
 * @brief 分離型ボックスぼかしシェーダー（縦または横の1次元パス）
 */



ConstantBuffer<SmoothingParams> gSmoothing : register(b0);
SamplerState gSampler : register(s0);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    int32_t halfSize = gSmoothing.kernelSize / 2;
    output.color.rgb = ApplyBoxBlur1D(gTexture, gSampler, input.texcoord, gSmoothing.direction, halfSize);
    output.color.a = 1.0f;
    
    return output;
}
