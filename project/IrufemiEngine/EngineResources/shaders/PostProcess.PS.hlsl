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
    
    // Kaleidoscope
    float32_t kaleidoscopeSegments;
    float32_t3 pad_kaleidoscope;

    // ChromaticAberration
    float32_t chromaticAberrationIntensity;
    float32_t3 pad_chromaticAberration;

    // DisplacementMap
    float32_t displacementMapIntensity;
    float32_t displacementMapTime;
    float32_t displacementMapTimeScale;
    float32_t pad_displacementMap;

    // DirectionalBlur
    float32_t2 directionalBlurDirection;
    float32_t directionalBlurStrength;
    int directionalBlurSamples;

    // Halftone
    float32_t halftoneScale;
    float32_t halftoneAngle;
    float32_t halftoneBlend;
    float32_t pad_halftone;

    // DepthOfField
    float32_t dofFocusDistance;
    float32_t dofFocusRange;
    float32_t dofBlurSize;
    int32_t dofSamples;
};

struct CustomEffectParams {
    float32_t4 color1;
    float32_t4 color2;
    float32_t param1;
    float32_t param2;
    float32_t param3;
    float32_t param4;
};

cbuffer CustomEffectParamsBuffer : register(b3) {
    CustomEffectParams gCustomParams[256];
};

ConstantBuffer<PostProcessParams> gParams : register(b0);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    float32_t4 color = gTexture.Sample(gSampler, input.texcoord);
    float32_t4 mask = gMaskTexture.Sample(gSampler, input.texcoord);
    
    float32_t2 uv = input.texcoord;
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = rcp(float32_t2(width, height));

    // マスク情報の復元
    int customEffect = round(mask.r * 255.0f);
    float customParam = mask.g;
    bool isProtected = mask.b > 0.5f;
    
    // 1. 個別カスタムエフェクトの適用
    if (customEffect != 0) {
        int instanceID = round(customParam * 255.0f);
        CustomEffectParams cParams;
        if (instanceID > 0 && instanceID < 256) {
            cParams = gCustomParams[instanceID];
        } else {
            cParams.color1 = float32_t4(1,1,1,1); cParams.color2 = float32_t4(0,0,0,1);
            cParams.param1 = 0; cParams.param2 = 0; cParams.param3 = 0; cParams.param4 = 0;
        }

        switch (customEffect) {
            case kPostProcessMode_Grayscale:
                color.rgb = ApplyGrayscale(color.rgb);
                break;
            case kPostProcessMode_Sepia:
                color.rgb = ApplySepia(color.rgb);
                break;
            case kPostProcessMode_Vignette:
                color.rgb = ApplyVignette(color.rgb, uv, instanceID > 0 ? cParams.param1 : gParams.vignetteRadius, instanceID > 0 ? cParams.param2 : gParams.vignetteSoftness, instanceID > 0 ? cParams.color1.rgb : gParams.vignetteColor.rgb);
                break;

            case kPostProcessMode_DepthBasedOutline:
                color.rgb = ApplyDepthBasedOutline(color.rgb, uv, uvStepSize, gParams.projectionInverse, instanceID > 0 ? cParams.param1 : gParams.outlineIntensity, gExtraTexture, gSamplerPoint);
                break;
            case kPostProcessMode_RadialBlur:
                color.rgb = ApplyRadialBlur(color.rgb, uv, gParams.radialBlurCenter, instanceID > 0 ? cParams.param1 : gParams.radialBlurWidth, gParams.radialBlurSamples, gTexture, gSampler);
                break;
            case kPostProcessMode_Dissolve:
                {
                    float32_t dmask = gExtraTexture.Sample(gSampler, uv).r;
                    float32_t3 res = ApplyDissolve(color.rgb, dmask, instanceID > 0 ? cParams.param1 : gParams.dissolveThreshold, instanceID > 0 ? cParams.param2 : gParams.dissolveEdgeRange, instanceID > 0 ? cParams.color1.rgb : gParams.dissolveEdgeColor.rgb);
                    if (res.r < 0) {
                        PixelShaderOutput outColor;
                        outColor.color = float32_t4(instanceID > 0 ? cParams.color2.rgb : gParams.dissolveBackgroundColor.rgb, 1.0f);
                        return outColor;
                    }
                    color.rgb = res;
                }
                break;
            case kPostProcessMode_Noise:
                color.rgb = ApplyNoise(color.rgb, uv, instanceID > 0 ? cParams.param1 : gParams.noiseIntensity, gParams.noiseTime);
                break;
            case kPostProcessMode_HSV:
                color.rgb = ApplyHSV(color.rgb, gParams.hsvHue, gParams.hsvSaturation, gParams.hsvValue);
                break;
            case kPostProcessMode_ToneMapping:
                color.rgb = ApplyToneMapping(color.rgb, instanceID > 0 ? cParams.param1 : gParams.toneMappingExposure);
                break;
            case kPostProcessMode_Fade:
                color.rgb = ApplyFade(color.rgb, instanceID > 0 ? cParams.color1.rgb : gParams.fadeColor.rgb, instanceID > 0 ? cParams.param1 : gParams.fadeIntensity);
                break;
            case kPostProcessMode_Slide:
                color.rgb = ApplySlide(color.rgb, uv, instanceID > 0 ? cParams.color1.rgb : gParams.slideColor.rgb, instanceID > 0 ? cParams.param1 : gParams.slideThreshold);
                break;
            case kPostProcessMode_Glitch:
                color.rgb = ApplyGlitch(color.rgb, uv, gParams.glitchTime, instanceID > 0 ? cParams.param1 : gParams.glitchIntensity, gTexture, gSampler, gMaskTexture, customEffect);
                break;
            case kPostProcessMode_LuminanceBasedOutline:
                color.rgb = ApplyLuminanceBasedOutline(color.rgb, uv, uvStepSize, instanceID > 0 ? cParams.param1 : gParams.luminanceOutlineThreshold, instanceID > 0 ? cParams.color1 : gParams.luminanceOutlineColor, gTexture, gSampler);
                break;
            case kPostProcessMode_Pixelation:
                color.rgb = ApplyPixelation(uv, instanceID > 0 ? cParams.param1 : gParams.pixelationSize, float32_t2(width, height), gTexture, gSampler);
                break;
            case kPostProcessMode_Pointillism:
                color.rgb = ApplyPointillism(uv, instanceID > 0 ? cParams.param1 : gParams.pointillismStrokeSize, gParams.pointillismColorSteps, gTexture, gSampler);
                break;
            case kPostProcessMode_Posterization:
                color.rgb = ApplyPosterization(color.rgb, instanceID > 0 ? cParams.param1 : gParams.posterizationSteps);
                break;
            case kPostProcessMode_NightVision:
                color.rgb = ApplyNightVision(color.rgb, uv, gParams.nightVisionTime, instanceID > 0 ? cParams.param1 : gParams.nightVisionIntensity);
                break;
            case kPostProcessMode_Kaleidoscope:
                color.rgb = ApplyKaleidoscope(uv, instanceID > 0 ? cParams.param1 : gParams.kaleidoscopeSegments, gTexture, gSampler);
                break;
            case kPostProcessMode_ChromaticAberration:
                color.rgb = ApplyChromaticAberration(uv, instanceID > 0 ? cParams.param1 : gParams.chromaticAberrationIntensity, gTexture, gSampler);
                break;
            case kPostProcessMode_DisplacementMap:
                color.rgb = ApplyDisplacementMap(uv, gParams.displacementMapTime, instanceID > 0 ? cParams.param1 : gParams.displacementMapIntensity, gTexture, gSampler);
                break;
            case kPostProcessMode_DirectionalBlur:
                color.rgb = ApplyDirectionalBlur(uv, gParams.directionalBlurDirection, instanceID > 0 ? cParams.param1 : gParams.directionalBlurStrength, gParams.directionalBlurSamples, gTexture, gSampler);
                break;
            case kPostProcessMode_Halftone:
                color.rgb = ApplyHalftone(color.rgb, uv, float32_t2(width, height), instanceID > 0 ? cParams.param1 : gParams.halftoneScale, gParams.halftoneAngle, gParams.halftoneBlend);
                break;
            case kPostProcessMode_DepthOfField:
                color.rgb = ApplyDepthOfField(color.rgb, uv, gExtraTexture, gSamplerPoint, gSampler, gParams.projectionInverse, instanceID > 0 ? cParams.param1 : gParams.dofFocusDistance, gParams.dofFocusRange, gParams.dofBlurSize, gParams.dofSamples, float32_t2(width, height), gTexture);
                break;
        }
    }
    
    // 2. 保護フラグの判定 (保護されていればここで終了)
    if (isProtected) {
        PixelShaderOutput output;
        output.color = color;
        return output;
    }

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
                color.rgb = ApplyGlitch(color.rgb, uv, gParams.glitchTime, gParams.glitchIntensity, gTexture, gSampler, gMaskTexture, 0);
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
                
            case kPostProcessMode_Kaleidoscope:
                color.rgb = ApplyKaleidoscope(uv, gParams.kaleidoscopeSegments, gTexture, gSampler);
                break;
                
            case kPostProcessMode_ChromaticAberration:
                color.rgb = ApplyChromaticAberration(uv, gParams.chromaticAberrationIntensity, gTexture, gSampler);
                break;
                
            case kPostProcessMode_DisplacementMap:
                color.rgb = ApplyDisplacementMap(uv, gParams.displacementMapTime, gParams.displacementMapIntensity, gTexture, gSampler);
                break;

            case kPostProcessMode_DirectionalBlur:
                color.rgb = ApplyDirectionalBlur(uv, gParams.directionalBlurDirection, gParams.directionalBlurStrength, gParams.directionalBlurSamples, gTexture, gSampler);
                break;
                
            case kPostProcessMode_Halftone:
                color.rgb = ApplyHalftone(color.rgb, uv, float32_t2(width, height), gParams.halftoneScale, gParams.halftoneAngle, gParams.halftoneBlend);
                break;
                
            case kPostProcessMode_DepthOfField:
                color.rgb = ApplyDepthOfField(color.rgb, uv, gExtraTexture, gSamplerPoint, gSampler, gParams.projectionInverse, gParams.dofFocusDistance, gParams.dofFocusRange, gParams.dofBlurSize, gParams.dofSamples, float32_t2(width, height), gTexture);
                break;
        }
    }

    PixelShaderOutput output;
    output.color = color;
    return output;
}
