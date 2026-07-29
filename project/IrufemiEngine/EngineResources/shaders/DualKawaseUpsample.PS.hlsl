#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);

struct DualKawaseBlurParams {
    float blurRadius;
    float intensity;
    int iterationCount;
    float pad;
};
ConstantBuffer<DualKawaseBlurParams> gKawaseParams : register(b0);



PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 texelSize = 1.0f / float2(width, height);
    
    float2 uv = input.texcoord;
    float x = texelSize.x * gKawaseParams.blurRadius;
    float y = texelSize.y * gKawaseParams.blurRadius;

    // 9-tap tent filter
    float4 sum = 0.0f;
    sum += gTexture.Sample(gSampler, uv + float2(-x, y)) * (1.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv + float2(0, y)) * (2.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv + float2(x, y)) * (1.0f / 16.0f);
    
    sum += gTexture.Sample(gSampler, uv + float2(-x, 0)) * (2.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv) * (4.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv + float2(x, 0)) * (2.0f / 16.0f);
    
    sum += gTexture.Sample(gSampler, uv + float2(-x, -y)) * (1.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv + float2(0, -y)) * (2.0f / 16.0f);
    sum += gTexture.Sample(gSampler, uv + float2(x, -y)) * (1.0f / 16.0f);

    output.color = sum * gKawaseParams.intensity;
    return output;
}
