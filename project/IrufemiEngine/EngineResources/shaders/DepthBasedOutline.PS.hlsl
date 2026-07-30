#include "Fullscreen.hlsli"
#include "Bindless.hlsli"
#include "PostProcessBindlessParams.hlsli"
#include "SpaceTransforms.hlsli"

struct OutlineParams {
    float32_t intensity;
    float32_t3 pad;
    float32_t4x4 projectionInverse;
};

ConstantBuffer<OutlineParams> gOutline : register(b0);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);



static const float32_t kPrewittHorizontalKernel[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

static const int32_t2 kIndex3x3[3][3] = {
    {{-1, -1}, {0, -1}, {1, -1}},
    {{-1,  0}, {0,  0}, {1,  0}},
    {{-1,  1}, {0,  1}, {1,  1}},
};

PixelShaderOutput main(VertexShaderOutput input) {
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(1.0f / width, 1.0f / height);

    float32_t2 depthDiff = float32_t2(0.0f, 0.0f);
    float32_t3 normalDiffX = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t3 normalDiffY = float32_t3(0.0f, 0.0f, 0.0f);
    
    for (int32_t x = 0; x < 3; ++x) {
        for (int32_t y = 0; y < 3; ++y) {
            float32_t2 texcoord = input.texcoord + kIndex3x3[x][y] * uvStepSize;
            
            // 深度のサンプリング
            float32_t ndcDepth = gExtraTexture.Sample(gSamplerPoint, texcoord).r;
            float32_t viewZ = ReconstructViewZ(ndcDepth, gOutline.projectionInverse);
            
            // 法線のサンプリング
            float32_t3 normal = gNormalTexture.Sample(gSamplerPoint, texcoord).xyz;
            
            depthDiff.x += viewZ * kPrewittHorizontalKernel[x][y];
            depthDiff.y += viewZ * kPrewittVerticalKernel[x][y];
            
            normalDiffX += normal * kPrewittHorizontalKernel[x][y];
            normalDiffY += normal * kPrewittVerticalKernel[x][y];
        }
    }

    // 変化の長さをウェイトとして合成
    float32_t depthWeight = length(depthDiff);
    float32_t normalWeight = length(normalDiffX) + length(normalDiffY);
    
    // 深度と法線のエッジをそれぞれ適度な係数で合成（法線の変化は1.0程度の差が出やすいため微調整）
    float32_t weight = saturate((depthWeight + normalWeight * 0.5f) * gOutline.intensity);

    PixelShaderOutput output;
    // エッジ部分を黒く表示するように合成
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    
    return output;
}
