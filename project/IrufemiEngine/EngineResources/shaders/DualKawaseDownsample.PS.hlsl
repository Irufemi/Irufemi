#include "PostProcessParameters.hlsli"
#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);


ConstantBuffer<DualKawaseBlurParams> gKawaseParams : register(b0);



PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 halfPixel = 0.5f / float2(width, height);
    float2 uv = input.texcoord;
    
    // 13-tap (simplified to 5 samples by using bilinear filtering)
    float4 sum = gTexture.Sample(gSampler, uv) * 4.0f;
    sum += gTexture.Sample(gSampler, uv - halfPixel);
    sum += gTexture.Sample(gSampler, uv + halfPixel);
    sum += gTexture.Sample(gSampler, uv + float2(halfPixel.x, -halfPixel.y));
    sum += gTexture.Sample(gSampler, uv + float2(-halfPixel.x, halfPixel.y));

    output.color = sum / 8.0f;
    return output;
}
