#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"

// --- 定数バッファ定義 ---
struct PostProcessParams {
    int32_t effectCount;
    int4 effects[4];      // C++側の int32_t[16] と完全に一致させるため int4[4] に変更
    
    // Vignette
    float32_t4 vignetteColor;
    float32_t vignetteScale;
    float32_t vignettePower;
    float32_t2 pad1;
    
    // Noise
    float32_t noiseIntensity;
    float32_t noiseTime;
    
    // Dissolve
    float32_t4 dissolveEdgeColor;
    float32_t4 dissolveBackgroundColor;
    float32_t dissolveThreshold;
    float32_t dissolveEdgeRange;
    
    // HSV
    float32_t hsvHue;
    float32_t hsvSaturation;
    float32_t hsvValue;
    
    // ToneMapping
    float32_t toneMappingExposure;
    
    // Fade
    float32_t4 fadeColor;
    float32_t fadeIntensity;
    
    // Slide
    float32_t4 slideColor;
    float32_t slideThreshold;
    
    // Outline
    float32_t4x4 projectionInverse;
    
    // Smoothing / Gaussian
    float32_t gaussianSigma;
    int32_t gaussianKernelSize;
    int32_t smoothingKernelSize;
    
    // RadialBlur
    float32_t2 radialBlurCenter;
    float32_t radialBlurWidth;
    int32_t radialBlurSamples;
    
    // Glitch
    float32_t glitchIntensity;
    float32_t glitchTime;
};

ConstantBuffer<PostProcessParams> gParams : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gExtraTexture : register(t1); // Depth or Mask
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    float32_t4 color = gTexture.Sample(gSampler, input.texcoord);
    float32_t2 uv = input.texcoord;

    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = rcp(float32_t2(width, height));

    for (int32_t i = 0; i < gParams.effectCount; ++i) {
        int32_t mode = gParams.effects[i / 4][i % 4];

        switch (mode) {
            case kPostProcessMode_Grayscale:
                color.rgb = ApplyGrayscale(color.rgb);
                break;
            
            case kPostProcessMode_Sepia:
                color.rgb = ApplySepia(color.rgb);
                break;
            
            case kPostProcessMode_Vignette:
                color.rgb = ApplyVignette(color.rgb, uv, gParams.vignetteScale, gParams.vignettePower, gParams.vignetteColor.rgb);
                break;

            case kPostProcessMode_Noise:
                color.rgb = ApplyNoise(color.rgb, uv, gParams.noiseIntensity, gParams.noiseTime);
                break;

            case kPostProcessMode_HSV:
                color.rgb = ApplyHSV(color.rgb, gParams.hsvHue, gParams.hsvSaturation, gParams.hsvValue);
                break;

            case kPostProcessMode_ToneMapping:
                color.rgb = ApplyToneMapping(color.rgb, gParams.toneMappingExposure);
                break;

            case kPostProcessMode_Fade:
                color.rgb = ApplyFade(color.rgb, gParams.fadeColor.rgb, gParams.fadeIntensity);
                break;

            case kPostProcessMode_Slide:
                color.rgb = ApplySlide(color.rgb, uv, gParams.slideColor.rgb, gParams.slideThreshold);
                break;

            case kPostProcessMode_Dissolve:
                {
                    float32_t mask = gExtraTexture.Sample(gSampler, uv);
                    float32_t3 res = ApplyDissolve(color.rgb, mask, gParams.dissolveThreshold, gParams.dissolveEdgeRange, gParams.dissolveEdgeColor.rgb);
                    if (res.r < 0) return (PixelShaderOutput)gParams.dissolveBackgroundColor;
                    color.rgb = res;
                }
                break;

            case kPostProcessMode_DepthBasedOutline:
                {
                    // Prewitt カーネルによるエッジ抽出 (簡易実装)
                    float32_t2 difference = 0;
                    for (int x = -1; x <= 1; ++x) {
                        for (int y = -1; y <= 1; ++y) {
                            float32_t depth = gExtraTexture.Sample(gSamplerPoint, uv + float32_t2(x, y) * uvStepSize);
                            float32_t4 viewSpace = mul(float32_t4(0, 0, depth, 1), gParams.projectionInverse);
                            float32_t vz = viewSpace.z / viewSpace.w;
                            
                            // 簡易的な Prewitt 重み
                            float32_t wx = (x == 0) ? 0 : (x < 0 ? -1.0/6.0 : 1.0/6.0);
                            float32_t wy = (y == 0) ? 0 : (y < 0 ? -1.0/6.0 : 1.0/6.0);
                            difference.x += vz * wx;
                            difference.y += vz * wy;
                        }
                    }
                    float32_t weight = saturate(length(difference) * 6.0f);
                    color.rgb *= (1.0f - weight);
                }
                break;

            case kPostProcessMode_Smoothing:
                {
                    float32_t3 accum = 0;
                    int32_t radius = (gParams.smoothingKernelSize - 1) / 2;
                    for (int32_t x = -radius; x <= radius; ++x) {
                        for (int32_t y = -radius; y <= radius; ++y) {
                            accum += gTexture.Sample(gSampler, uv + float32_t2(x, y) * uvStepSize).rgb;
                        }
                    }
                    color.rgb = accum / (float32_t(gParams.smoothingKernelSize * gParams.smoothingKernelSize));
                }
                break;

            case kPostProcessMode_GaussianFilter:
                {
                    float32_t3 sum = 0;
                    float32_t totalW = 0;
                    int32_t half = gParams.gaussianKernelSize / 2;
                    for (int32_t x = -half; x <= half; ++x) {
                        for (int32_t y = -half; y <= half; ++y) {
                            float32_t w = gauss(float32_t(x), float32_t(y), gParams.gaussianSigma);
                            sum += gTexture.Sample(gSampler, uv + float32_t2(x, y) * uvStepSize).rgb * w;
                            totalW += w;
                        }
                    }
                    color.rgb = sum / totalW;
                }
                break;

            case kPostProcessMode_RadialBlur:
                {
                    float32_t2 dir = uv - gParams.radialBlurCenter;
                    float32_t3 sum = 0;
                    for (int32_t j = 0; j < gParams.radialBlurSamples; ++j) {
                        sum += gTexture.Sample(gSampler, uv + dir * gParams.radialBlurWidth * float32_t(j)).rgb;
                    }
                    color.rgb = sum / float32_t(gParams.radialBlurSamples);
                }
                break;

            case kPostProcessMode_Glitch:
                color.rgb = ApplyGlitch(color.rgb, uv, gParams.glitchTime, gParams.glitchIntensity, gTexture, gSampler);
                break;
        }
    }

    PixelShaderOutput output;
    output.color = color;
    return output;
}
