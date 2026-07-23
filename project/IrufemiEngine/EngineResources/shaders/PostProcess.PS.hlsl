#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"

// --- 定数バッファ定義 ---
struct PostProcessParams {
    int32_t effectCount;
    int4 effects[4];      // C++側の int32_t[16] と完全に一致させるため int4[4] に変更
    
    // Vignette
    float32_t4 vignetteColor;
    float32_t vignetteRadius;
    float32_t vignetteSoftness;
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
    float32_t outlineIntensity;
    float32_t3 pad_outline;
    
    // RadialBlur
    float32_t2 radialBlurCenter;
    float32_t radialBlurWidth;
    int32_t radialBlurSamples;
    
    // Glitch
    float32_t glitchIntensity;
    float32_t glitchTime;
    float32_t2 pad_glitch;
    
    // LuminanceBasedOutline
    float32_t4 luminanceOutlineColor;
    float32_t luminanceOutlineThreshold;
    float32_t3 pad_lumOutline;
    
    // Pixelation
    float32_t pixelationSize;
    float32_t3 pad_pixelation;
    
    // Pointillism
    float32_t pointillismStrokeSize;
    float32_t pointillismColorSteps;
    float32_t2 pad_pointillism;
    
    // Posterization
    float32_t posterizationSteps;
    float32_t3 pad_posterization;
    
    // NightVision
    float32_t nightVisionIntensity;
    float32_t nightVisionTime;
    float32_t2 pad_nightVision;
};

ConstantBuffer<PostProcessParams> gParams : register(b0);
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
                color.rgb = ApplyVignette(color.rgb, uv, gParams.vignetteRadius, gParams.vignetteSoftness, gParams.vignetteColor.rgb);
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
                    float32_t mask = gExtraTexture.Sample(gSampler, uv).r;
                    float32_t3 res = ApplyDissolve(color.rgb, mask, gParams.dissolveThreshold, gParams.dissolveEdgeRange, gParams.dissolveEdgeColor.rgb);
                    if (res.r < 0) return (PixelShaderOutput)gParams.dissolveBackgroundColor;
                    color.rgb = res;
                }
                break;

            case kPostProcessMode_DepthBasedOutline:
                color.rgb = ApplyDepthBasedOutline(color.rgb, uv, uvStepSize, gParams.projectionInverse, gParams.outlineIntensity, gExtraTexture, gSamplerPoint);
                break;

            case kPostProcessMode_RadialBlur:
                color.rgb = ApplyRadialBlur(color.rgb, uv, gParams.radialBlurCenter, gParams.radialBlurWidth, gParams.radialBlurSamples, gTexture, gSampler);
                break;

            case kPostProcessMode_Glitch:
                color.rgb = ApplyGlitch(color.rgb, uv, gParams.glitchTime, gParams.glitchIntensity, gTexture, gSampler);
                break;
                
            case kPostProcessMode_LuminanceBasedOutline:
                color.rgb = ApplyLuminanceBasedOutline(color.rgb, uv, uvStepSize, gParams.luminanceOutlineThreshold, gParams.luminanceOutlineColor, gTexture, gSampler);
                break;
                
            case kPostProcessMode_Pixelation:
                color.rgb = ApplyPixelation(uv, gParams.pixelationSize, float32_t2(width, height), gTexture, gSampler);
                break;
                
            case kPostProcessMode_Pointillism:
                color.rgb = ApplyPointillism(uv, gParams.pointillismStrokeSize, gParams.pointillismColorSteps, gTexture, gSampler);
                break;
                
            case kPostProcessMode_Posterization:
                color.rgb = ApplyPosterization(color.rgb, gParams.posterizationSteps);
                break;
                
            case kPostProcessMode_NightVision:
                color.rgb = ApplyNightVision(color.rgb, uv, gParams.nightVisionTime, gParams.nightVisionIntensity);
                break;
        }
    }

    PixelShaderOutput output;
    output.color = color;
    return output;
}
