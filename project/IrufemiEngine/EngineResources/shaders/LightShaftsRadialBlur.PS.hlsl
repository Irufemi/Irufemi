#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"
struct LightShaftsParams {
    float32_t2 lightScreenPos;
    float32_t density;
    float32_t decay;
    float32_t weight;
    float32_t exposure;
    int32_t samples;
    float32_t pad;
};

ConstantBuffer<LightShaftsParams> gParams : register(b0);
SamplerState gSampler : register(s0);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    float2 deltaTextCoord = input.texcoord - gParams.lightScreenPos;
    deltaTextCoord *= (1.0f / float(gParams.samples)) * gParams.density;
    
    float2 textCoord = input.texcoord;
    float illuminationDecay = 1.0f;
    float3 shaftColor = 0.0f;
    
    for (int i = 0; i < gParams.samples; i++) {
        textCoord -= deltaTextCoord;
        float3 sampleColor = gTexture.SampleLevel(gSampler, textCoord, 0).rgb;
        sampleColor *= illuminationDecay * gParams.weight;
        shaftColor += sampleColor;
        illuminationDecay *= gParams.decay;
    }
    
    output.color = float32_t4(shaftColor * gParams.exposure, 1.0f);
    return output;
}
