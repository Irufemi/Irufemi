#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"

SamplerState gSampler : register(s0);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    float3 mainColor = gTexture.SampleLevel(gSampler, input.texcoord, 0).rgb;
    float3 lsColor = gExtraTexture.SampleLevel(gSampler, input.texcoord, 0).rgb;
    
    // Additive Blend
    output.color = float32_t4(mainColor + lsColor, 1.0f);
    
    return output;
}
