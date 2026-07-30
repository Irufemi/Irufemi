#include "PostProcessParameters.hlsli"
#include "Fullscreen.hlsli"
#include "PostProcess.hlsli"

// --- 定数バッファ定義 ---




cbuffer CustomEffectParamsBuffer : register(b3) {
    CustomEffectParams gCustomParams[256];
};

ConstantBuffer<PostProcessParams> gParams : register(b0);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);



/**
 * @brief マスク（ID）ベースのカスタムアウトラインを適用する（AAAアプローチ）
 * 
 * 自身のピクセルだけでなく周囲のマスクIDをサンプリングし、エッジを検出します。
 * 境界と判定された場合、最も支配的なマスクIDに対応する CustomEffectParams を取得し、色と太さを適用します。
 */
float32_t3 ApplyMaskBasedOutline(float32_t3 color, float32_t2 uv, float32_t2 uvStepSize, Texture2D<float32_t4> maskTex, SamplerState smp) {
    float32_t4 centerMask = maskTex.SampleLevel(smp, uv, 0);
    int centerEffect = round(centerMask.r * 255.0f);
    int centerInstance = round(centerMask.g * 255.0f);

    float maxWeight = 0.0f;
    float32_t3 outlineColor = color;
    
    const int kRadius = 3;

    for (int y = -kRadius; y <= kRadius; y++) {
        for (int x = -kRadius; x <= kRadius; x++) {
            if (x == 0 && y == 0) continue;
            
            float dist = sqrt(float(x * x + y * y));
            if (dist > float(kRadius)) continue;

            float32_t2 offset = float32_t2(x, y) * uvStepSize;
            float32_t4 neighborMask = maskTex.SampleLevel(smp, uv + offset, 0);
            int neighborEffect = round(neighborMask.r * 255.0f);
            int neighborInstance = round(neighborMask.g * 255.0f);

            // 自分と異なるオブジェクトで、アウトライン対象のエフェクトを持っているか
            if (neighborInstance != centerInstance && neighborInstance > 0 && neighborInstance < 256) {
                if (neighborEffect == kPostProcessMode_DepthBasedOutline || neighborEffect == kPostProcessMode_LuminanceBasedOutline) {
                    CustomEffectParams cParams = gCustomParams[neighborInstance];
                    float thickness = max(1.5f, cParams.param1); // 斜めピクセル(1.414)をカバーするため最低1.5
                    if (dist <= thickness) {
                        float effectAlpha = cParams.color1.a > 0.0f ? cParams.color1.a : 1.0f; // Alpha0の時は強制的に1.0にする
                        float alpha = (1.0f - smoothstep(thickness - 0.5f, thickness + 0.5f, dist)) * effectAlpha;
                        if (alpha > maxWeight) {
                            maxWeight = alpha;
                            outlineColor = cParams.color1.rgb;
                        }
                    }
                }
            }
        }
    }

    // 内側のアウトライン（自身がアウトライン対象で、周囲が背景または別オブジェクトの場合）
    if ((centerEffect == kPostProcessMode_DepthBasedOutline || centerEffect == kPostProcessMode_LuminanceBasedOutline) && centerInstance > 0 && centerInstance < 256) {
        CustomEffectParams cParams = gCustomParams[centerInstance];
        float thickness = max(1.5f, cParams.param1);
        
        for (int y = -kRadius; y <= kRadius; y++) {
            for (int x = -kRadius; x <= kRadius; x++) {
                if (x == 0 && y == 0) continue;
                float dist = sqrt(float(x * x + y * y));
                if (dist > thickness) continue;

                float32_t2 offset = float32_t2(x, y) * uvStepSize;
                float32_t4 neighborMask = maskTex.SampleLevel(smp, uv + offset, 0);
                int neighborInstance = round(neighborMask.g * 255.0f);

                if (neighborInstance != centerInstance) {
                    float effectAlpha = cParams.color1.a > 0.0f ? cParams.color1.a : 1.0f;
                    float alpha = (1.0f - smoothstep(thickness - 0.5f, thickness + 0.5f, dist)) * effectAlpha;
                    if (alpha > maxWeight) {
                        maxWeight = alpha;
                        outlineColor = cParams.color1.rgb;
                    }
                }
            }
        }
    }

    if (maxWeight > 0.0f) {
        return lerp(color, outlineColor, maxWeight);
    }
    return color;
}

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
    
    PixelShaderOutput output;
    
    // 0. 全画面でのマスクベースのアウトラインエッジ検出 (AAA Approach)
    // 背景ピクセルであっても、隣接するピクセルがアウトライン対象であれば描画する
    color.rgb = ApplyMaskBasedOutline(color.rgb, uv, uvStepSize, gMaskTexture, gSamplerPoint);
    
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

    output.color = color;
    return output;
}
