#include "Fullscreen.hlsli"

struct VignetteParams {
    float4 color;
    float scale;
    float power;
    float2 pad;
};

ConstantBuffer<VignetteParams> gVignette : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // 周囲を0に、中心になるほど明るくなるように計算で調整
    float2 correct = input.texcoord * (1.0f - input.texcoord.yx);
    // Scaleで調整
    float vignette = correct.x * correct.y * gVignette.scale;
    // べき乗で調整
    vignette = saturate(pow(vignette, gVignette.power));
    // 係数として補間
    output.color.rgb = lerp(gVignette.color.rgb, output.color.rgb, vignette);

    return output;
}
