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
SamplerState gSamplerPoint : register(s1);

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VertexShaderOutput input) {
    PSOutput output;
    
    float32_t4 color = gTexture.SampleLevel(gSampler, input.texcoord, 0);
    float depth = gExtraTexture.SampleLevel(gSamplerPoint, input.texcoord, 0).r;
    
    float dist = distance(input.texcoord, gParams.lightScreenPos);
    float sunMask = 1.0f - smoothstep(0.0f, 0.4f, dist);
    
    // 空（深度が一番奥）かつ、光源に近い部分だけを抽出
    float extractMask = (depth >= 0.9999f) ? sunMask : 0.0f;
    
    // 非常に明るいピクセルも抽出
    float luminance = dot(color.rgb, float32_t3(0.2126, 0.7152, 0.0722));
    float brightMask = saturate(luminance - 0.9f); 

    // マスクを合成して出力
    float mask = saturate(extractMask + brightMask);
    output.color = float32_t4(color.rgb * mask, 1.0f);
    
    return output;
}
