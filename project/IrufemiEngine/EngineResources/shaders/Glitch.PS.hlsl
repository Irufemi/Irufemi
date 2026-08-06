#include "PostProcessParameters.hlsli"
#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"



ConstantBuffer<GlitchParams> gParams : register(b0);
SamplerState gSampler : register(s0);



PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // 現在のカラーを取得
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 maskColor = gMaskTexture.Sample(gSampler, input.texcoord);
    
    if (maskColor.r > 0.5f) {
        output.color = textureColor;
        return output;
    }
    
    // ApplyGlitchで処理
    output.color.rgb = ApplyGlitch(textureColor.rgb, input.texcoord, gParams, gTexture, gSampler, gMaskTexture, 0);
    output.color.a = textureColor.a;
    
    return output;
}
