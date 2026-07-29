#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);

struct FadeParams {
    float32_t4 color;
    float32_t intensity;
};

ConstantBuffer<FadeParams> gParams : register(b0);



PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // 元のテクスチャカラーを取得
    float32_t4 texColor = gTexture.Sample(gSampler, input.texcoord);
    
    // 指定した色と元の色を強度(intensity)で線形補間
    output.color.rgb = lerp(texColor.rgb, gParams.color.rgb, gParams.intensity);
    output.color.a = texColor.a;
    
    return output;
}
