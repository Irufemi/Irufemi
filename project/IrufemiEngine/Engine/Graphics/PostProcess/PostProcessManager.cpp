/**
 * @file PostProcessManager.cpp
 * @brief ポストプロセス（マルチパス描画）の管理実装クラス
 */
#include "PostProcessManager.h"
#include "../DirectX/DirectXCommon.h"
#include "../DirectX/DirectXUtils.h"
#include "../../IrufemiEngine.h"
#include <algorithm>
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h>
#include <iostream>
#include <string>
#include <wrl/client.h>

namespace {
    bool RequiresSeparatePass(PostProcessManager::Mode mode) {
        // 空間サンプリングやUV座標を操作するエフェクトは、
        // 以前のエフェクト結果がテクスチャに完全に書き込まれている必要があるため独立パスとする
        return (mode == PostProcessManager::Mode::GaussianFilter ||
                mode == PostProcessManager::Mode::DepthBasedOutline ||
                mode == PostProcessManager::Mode::RadialBlur ||
                mode == PostProcessManager::Mode::Glitch ||
                mode == PostProcessManager::Mode::DualKawaseBlur ||
                mode == PostProcessManager::Mode::Pointillism ||
                mode == PostProcessManager::Mode::Kaleidoscope ||
                mode == PostProcessManager::Mode::ChromaticAberration ||
                mode == PostProcessManager::Mode::DisplacementMap ||
                mode == PostProcessManager::Mode::DirectionalBlur ||
                mode == PostProcessManager::Mode::DepthOfField);
    }
}

void PostProcessManager::Initialize(DirectXCommon* dxCommon,
                                    DXGI_FORMAT rtvFormat) {
  dxCommon_ = dxCommon;
  device_ = dxCommon->GetDevice();
  rootSig_ = dxCommon->GetRootSignature();
  rtvFormat_ = rtvFormat;

  CreateConstantBuffers();
  CreatePSOs();
}

void PostProcessManager::ResetAllParams() {
    noiseParams_ = NoiseParams();
    vignetteParams_ = VignetteParams();
    smoothingParams_ = SmoothingParams();
    gaussianParams_ = GaussianParams();
    radialBlurParams_ = RadialBlurParams();
    outlineParams_ = OutlineParams();
    dissolveParams_ = DissolveParams();
    hsvParams_ = HSVParams();
    toneMappingParams_ = ToneMappingParams();
    fadeParams_ = FadeParams();
    slideParams_ = SlideParams();
    bloomParams_ = BloomParams();
    glitchParams_ = GlitchParams();
    dualKawaseParams_ = DualKawaseBlurParams();
    luminanceOutlineParams_ = LuminanceOutlineParams();
    pixelationParams_ = PixelationParams();
    pointillismParams_ = PointillismParams();
    posterizationParams_ = PosterizationParams();
    nightVisionParams_ = NightVisionParams();
    kaleidoscopeParams_ = KaleidoscopeParams();
    chromaticAberrationParams_ = ChromaticAberrationParams();
    displacementMapParams_ = DisplacementMapParams();
    directionalBlurParams_ = DirectionalBlurParams();
    halftoneParams_ = HalftoneParams();
    dofParams_ = DepthOfFieldParams();
}



void PostProcessManager::Update(float totalTime) {
  CommitPendingModes();

  noiseParams_.time = totalTime;
  nightVisionParams_.time = totalTime;
  if (mappedNoise_) {
    *mappedNoise_ = noiseParams_;
  }

  // 他のパラメータも同期
  if (mappedVignette_) *mappedVignette_ = vignetteParams_;
  if (mappedSmoothing_) *mappedSmoothing_ = smoothingParams_;
  if (mappedGaussian_) *mappedGaussian_ = gaussianParams_;
  if (mappedRadialBlur_) *mappedRadialBlur_ = radialBlurParams_;
  if (mappedOutline_) *mappedOutline_ = outlineParams_;
  if (mappedDissolve_) *mappedDissolve_ = dissolveParams_;
  if (mappedHsv_) *mappedHsv_ = hsvParams_;
  if (mappedToneMapping_) *mappedToneMapping_ = toneMappingParams_;
  if (mappedFade_) *mappedFade_ = fadeParams_;
  if (mappedSlide_) *mappedSlide_ = slideParams_;
  if (mappedBloom_) *mappedBloom_ = bloomParams_;
  if (mappedLightShafts_) *mappedLightShafts_ = lightShaftsParams_;

  glitchParams_.time = totalTime;
  if (mappedGlitch_) *mappedGlitch_ = glitchParams_;

  if (mappedDualKawase_) *mappedDualKawase_ = dualKawaseParams_;

  // 統合パラメータの同期
  combinedParams_.vignetteColor = vignetteParams_.color;
  combinedParams_.vignetteRadius = vignetteParams_.radius;
  combinedParams_.vignetteSoftness = vignetteParams_.softness;
  combinedParams_.noiseIntensity = noiseParams_.intensity;
  combinedParams_.noiseTime = noiseParams_.time;
  combinedParams_.dissolveEdgeColor = dissolveParams_.edgeColor;
  combinedParams_.dissolveBackgroundColor = dissolveParams_.backgroundColor;
  combinedParams_.dissolveThreshold = dissolveParams_.threshold;
  combinedParams_.dissolveEdgeRange = dissolveParams_.edgeRange;
  combinedParams_.hsvHue = hsvParams_.hue;
  combinedParams_.hsvSaturation = hsvParams_.saturation;
  combinedParams_.hsvValue = hsvParams_.value;
  combinedParams_.toneMappingExposure = toneMappingParams_.exposure;
  combinedParams_.fadeColor = fadeParams_.color;
  combinedParams_.fadeIntensity = fadeParams_.intensity;
  combinedParams_.slideColor = slideParams_.color;
  combinedParams_.slideThreshold = slideParams_.threshold;
  combinedParams_.projectionInverse = outlineParams_.projectionInverse;
  combinedParams_.outlineIntensity = outlineParams_.intensity;
  combinedParams_.radialBlurCenter = radialBlurParams_.center;
  combinedParams_.radialBlurWidth = radialBlurParams_.blurWidth;
  combinedParams_.radialBlurSamples = radialBlurParams_.numSamples;
  combinedParams_.glitchIntensity = glitchParams_.intensity;
  combinedParams_.glitchTime = glitchParams_.time;
  
  combinedParams_.luminanceOutlineThreshold = luminanceOutlineParams_.threshold;
  combinedParams_.luminanceOutlineColor = luminanceOutlineParams_.outlineColor;

  combinedParams_.pixelationSize = pixelationParams_.pixelSize;

  combinedParams_.pointillismStrokeSize = pointillismParams_.strokeSize;
  combinedParams_.pointillismColorSteps = pointillismParams_.colorSteps;

  combinedParams_.posterizationSteps = posterizationParams_.colorSteps;
  
  combinedParams_.nightVisionIntensity = nightVisionParams_.intensity;
  combinedParams_.nightVisionTime = nightVisionParams_.time;
  
  combinedParams_.kaleidoscopeSegments = kaleidoscopeParams_.segments;
  
  combinedParams_.chromaticAberrationIntensity = chromaticAberrationParams_.intensity;
  
  displacementMapParams_.time = totalTime * displacementMapParams_.timeScale;
  combinedParams_.displacementMapIntensity = displacementMapParams_.intensity;
  combinedParams_.displacementMapTime = displacementMapParams_.time;
  combinedParams_.displacementMapTimeScale = displacementMapParams_.timeScale;

  combinedParams_.directionalBlurDirection = directionalBlurParams_.direction;
  combinedParams_.directionalBlurStrength = directionalBlurParams_.strength;
  combinedParams_.directionalBlurSamples = directionalBlurParams_.samples;

  combinedParams_.halftoneScale = halftoneParams_.scale;
  combinedParams_.halftoneAngle = halftoneParams_.angle;
  combinedParams_.halftoneBlend = halftoneParams_.blend;

  combinedParams_.dofFocusDistance = dofParams_.focusDistance;
  combinedParams_.dofFocusRange = dofParams_.focusRange;
  combinedParams_.dofBlurSize = dofParams_.blurSize;
  combinedParams_.dofSamples = dofParams_.samples;

  if (mappedCombined_) {
    *mappedCombined_ = combinedParams_;
  }
}

void PostProcessManager::Draw(ID3D12GraphicsCommandList *commandList,
                               RenderTexture *srcTexture,
                               D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                               const PostProcessWorkspace& workspace) {
  RenderTexture *currentSource = srcTexture;
  bindlessBufferOffset_ = 0; // フレーム開始時にリセット

  {
      std::lock_guard<std::mutex> lock(customParamsMutex_);
      size_t count = std::min<size_t>(customEffectParamsList_.size(), kMaxCustomEffectParams - 1);
      if (count > 0 && mappedCustomEffectParams_) {
          // ID 0 is reserved (no custom param), so start copying from index 1.
          // In customEffectParamsList_, index 0 corresponds to ID 1.
          std::memcpy(mappedCustomEffectParams_ + 1, customEffectParamsList_.data(), count * sizeof(CustomEffectParams));
      }
  }

  if (!activeModes_.empty()) {
    size_t modeIdx = 0;
    int pingPongIdx = 0;

    while (modeIdx < activeModes_.size()) {
      Mode mode = activeModes_[modeIdx];
      bool isLastBatch = false;

      // 1) 非統合エフェクト (Bloom) の処理
      if (mode == Mode::Bloom) {
        isLastBatch = (modeIdx == activeModes_.size() - 1);
        RenderTexture* nextTarget = isLastBatch ? nullptr : workspace.workTextures[pingPongIdx % 2];
        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle = isLastBatch ? rtvHandle : nextTarget->GetRtvHandle();

        if (!isLastBatch) {
            DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        RenderTexture* bloomExtract = workspace.bloomExtract;
        RenderTexture* blurH = workspace.bloomBlur;
        RenderTexture* blurV = workspace.bloomExtract;

        DirectXUtils::TransitionBarrier(commandList, bloomExtract->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DrawSinglePass(commandList, Mode::Bloom, currentSource, bloomExtract->GetRtvHandle(), false, bloomExtractPSO_.Get());
        DirectXUtils::TransitionBarrier(commandList, bloomExtract->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        bloomParams_.direction = { 1.0f, 0.0f };
        if (mappedBloom_) { *mappedBloom_ = bloomParams_; }
        DirectXUtils::TransitionBarrier(commandList, blurH->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DrawSinglePass(commandList, Mode::Bloom, bloomExtract, blurH->GetRtvHandle(), false, bloomBlurHPSO_.Get());
        DirectXUtils::TransitionBarrier(commandList, blurH->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        bloomParams_.direction = { 0.0f, 1.0f };
        if (mappedBloom_) { *mappedBloom_ = bloomParams_; }
        DirectXUtils::TransitionBarrier(commandList, blurV->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        DrawSinglePass(commandList, Mode::Bloom, blurH, blurV->GetRtvHandle(), false, bloomBlurVPSO_.Get());
        DirectXUtils::TransitionBarrier(commandList, blurV->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        
        commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(targetHandle, clearColor, 0, nullptr);
        commandList->SetPipelineState(isLastBatch ? finalBloomCombinePSO_.Get() : bloomCombinePSO_.Get());
        commandList->SetGraphicsRootSignature(rootSig_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // Bindless パラメータの更新
        mappedBindless_[bindlessBufferOffset_].mainTextureIndex = currentSource ? currentSource->GetSrvIndex() : 0;
        mappedBindless_[bindlessBufferOffset_].extraTextureIndex = blurV ? blurV->GetSrvIndex() : 0;
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
        bindlessBufferOffset_++;
        
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, bloomCB_->GetGPUVirtualAddress());
        commandList->DrawInstanced(3, 1, 0, 0);

        if (!isLastBatch) {
          DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
          pingPongIdx++;
        }
        currentSource = nextTarget;
        modeIdx++;
      }
      // 1-LS) Light Shafts (ゴッドレイ)
      else if (mode == Mode::LightShafts) {
        isLastBatch = (modeIdx == activeModes_.size() - 1);
        RenderTexture* nextTarget = isLastBatch ? nullptr : workspace.workTextures[pingPongIdx % 2];
        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle = isLastBatch ? rtvHandle : nextTarget->GetRtvHandle();

        if (!isLastBatch) {
            DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        RenderTexture* lsExtract = workspace.lsExtract;
        RenderTexture* lsBlur = workspace.lsBlur;

        // Pass 1: Extract (from currentSource & depth to lsExtract)
        DirectXUtils::TransitionBarrier(commandList, lsExtract->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE lsExtractRtv = lsExtract->GetRtvHandle();
        commandList->OMSetRenderTargets(1, &lsExtractRtv, false, nullptr);
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(lsExtractRtv, clearColor, 0, nullptr);
        commandList->SetPipelineState(lsExtractPSO_.Get());
        commandList->SetGraphicsRootSignature(rootSig_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mappedBindless_[bindlessBufferOffset_].mainTextureIndex = currentSource ? currentSource->GetSrvIndex() : 0;
        mappedBindless_[bindlessBufferOffset_].extraTextureIndex = depthSrvIndex_;
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
        bindlessBufferOffset_++;
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, lightShaftsCB_->GetGPUVirtualAddress());
        commandList->DrawInstanced(3, 1, 0, 0);
        DirectXUtils::TransitionBarrier(commandList, lsExtract->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // Pass 2: Radial Blur (from lsExtract to lsBlur)
        DirectXUtils::TransitionBarrier(commandList, lsBlur->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        D3D12_CPU_DESCRIPTOR_HANDLE lsBlurRtv = lsBlur->GetRtvHandle();
        commandList->OMSetRenderTargets(1, &lsBlurRtv, false, nullptr);
        commandList->ClearRenderTargetView(lsBlurRtv, clearColor, 0, nullptr);
        commandList->SetPipelineState(lsRadialBlurPSO_.Get());
        mappedBindless_[bindlessBufferOffset_].mainTextureIndex = lsExtract->GetSrvIndex();
        mappedBindless_[bindlessBufferOffset_].extraTextureIndex = 0;
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
        bindlessBufferOffset_++;
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, lightShaftsCB_->GetGPUVirtualAddress());
        commandList->DrawInstanced(3, 1, 0, 0);
        DirectXUtils::TransitionBarrier(commandList, lsBlur->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // Pass 3: Combine (from currentSource & lsBlur to targetHandle)
        commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
        commandList->ClearRenderTargetView(targetHandle, clearColor, 0, nullptr);
        commandList->SetPipelineState(isLastBatch ? finalLsCombinePSO_.Get() : lsCombinePSO_.Get());
        mappedBindless_[bindlessBufferOffset_].mainTextureIndex = currentSource ? currentSource->GetSrvIndex() : 0;
        mappedBindless_[bindlessBufferOffset_].extraTextureIndex = lsBlur->GetSrvIndex();
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
        bindlessBufferOffset_++;
        commandList->DrawInstanced(3, 1, 0, 0);

        if (!isLastBatch) {
          DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
          pingPongIdx++;
        }
        currentSource = nextTarget;
        modeIdx++;
      }
      // 1-B) 分離可能フィルタ (Smoothing / GaussianFilter) の処理
      else if (mode == Mode::Smoothing || mode == Mode::GaussianFilter) {
        isLastBatch = (modeIdx == activeModes_.size() - 1);
        RenderTexture* nextTarget = isLastBatch ? nullptr : workspace.workTextures[pingPongIdx % 2];
        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle = isLastBatch ? rtvHandle : nextTarget->GetRtvHandle();

        // 横方向パス用の中間バッファ（BloomのBlurバッファを一時的に借用）
        RenderTexture* blurH = workspace.bloomBlur;
        
        DirectXUtils::TransitionBarrier(commandList, blurH->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

        ID3D12PipelineState* psoH = (mode == Mode::Smoothing) ? smoothingBlurPSO_.Get() : gaussianBlurPSO_.Get();
        ID3D12PipelineState* psoV = nullptr;
        if (mode == Mode::Smoothing) {
            psoV = isLastBatch ? finalSmoothingBlurPSO_.Get() : smoothingBlurPSO_.Get();
        } else {
            psoV = isLastBatch ? finalGaussianBlurPSO_.Get() : gaussianBlurPSO_.Get();
        }

        // H Pass
        if (mode == Mode::Smoothing) {
            smoothingParams_.direction = { 1.0f, 0.0f };
            if (mappedSmoothing_) { *mappedSmoothing_ = smoothingParams_; }
        } else {
            gaussianParams_.direction = { 1.0f, 0.0f };
            if (mappedGaussian_) { *mappedGaussian_ = gaussianParams_; }
        }
        DrawSinglePass(commandList, mode, currentSource, blurH->GetRtvHandle(), false, psoH);
        
        DirectXUtils::TransitionBarrier(commandList, blurH->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // V Pass
        if (!isLastBatch) {
            DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        if (mode == Mode::Smoothing) {
            smoothingParams_.direction = { 0.0f, 1.0f };
            if (mappedSmoothing_) { *mappedSmoothing_ = smoothingParams_; }
        } else {
            gaussianParams_.direction = { 0.0f, 1.0f };
            if (mappedGaussian_) { *mappedGaussian_ = gaussianParams_; }
        }
        
        commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
        float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        commandList->ClearRenderTargetView(targetHandle, clearColor, 0, nullptr);
        DrawSinglePass(commandList, mode, blurH, targetHandle, isLastBatch, psoV);

        if (!isLastBatch) {
          DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
          pingPongIdx++;
        }
        currentSource = nextTarget;
        modeIdx++;
      }
      // 1-C) カワセブラー (DualKawaseBlur) の処理
      else if (mode == Mode::DualKawaseBlur) {
        isLastBatch = (modeIdx == activeModes_.size() - 1);
        RenderTexture* nextTarget = isLastBatch ? nullptr : workspace.workTextures[pingPongIdx % 2];
        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle = isLastBatch ? rtvHandle : nextTarget->GetRtvHandle();

        int32_t iterations = (std::min)(kMaxKawaseIterations, (std::max)(1, dualKawaseParams_.iterationCount));
        RenderTexture* prevSource = currentSource;
        
        // Downsample
        for (int i = 0; i < iterations; ++i) {
            RenderTexture* kwTex = workspace.kawaseTextures[i];
            if (!kwTex) break;
            
            D3D12_VIEWPORT viewport{};
            viewport.Width = (FLOAT)kwTex->GetWidth();
            viewport.Height = (FLOAT)kwTex->GetHeight();
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect{};
            scissorRect.right = kwTex->GetWidth();
            scissorRect.bottom = kwTex->GetHeight();
            commandList->RSSetScissorRects(1, &scissorRect);

            DirectXUtils::TransitionBarrier(commandList, kwTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            
            if (mappedDualKawase_) { *mappedDualKawase_ = dualKawaseParams_; }
            DrawSinglePass(commandList, Mode::DualKawaseBlur, prevSource, kwTex->GetRtvHandle(), false, dualKawaseDownsamplePSO_.Get());
            
            DirectXUtils::TransitionBarrier(commandList, kwTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            prevSource = kwTex;
        }

        // Upsample
        for (int i = iterations - 2; i >= 0; --i) {
            RenderTexture* kwTex = workspace.kawaseTextures[i];
            if (!kwTex && i != 0) continue; // i==0の場合は最終出力なのでkwTexは見ない
            
            bool isFinalUp = (i == 0);
            D3D12_CPU_DESCRIPTOR_HANDLE upHandle = isFinalUp ? targetHandle : kwTex->GetRtvHandle();
            
            uint32_t tw = isFinalUp ? dxCommon_->GetClientWidth() : kwTex->GetWidth();
            uint32_t th = isFinalUp ? dxCommon_->GetClientHeight() : kwTex->GetHeight();

            D3D12_VIEWPORT viewport{};
            viewport.Width = (FLOAT)tw;
            viewport.Height = (FLOAT)th;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect{};
            scissorRect.right = tw;
            scissorRect.bottom = th;
            commandList->RSSetScissorRects(1, &scissorRect);

            if (!isFinalUp) {
                DirectXUtils::TransitionBarrier(commandList, kwTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            } else if (!isLastBatch) {
                DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            
            ID3D12PipelineState* upPso = (isFinalUp && isLastBatch) ? finalDualKawaseUpsamplePSO_.Get() : dualKawaseUpsamplePSO_.Get();
            
            if (isFinalUp) {
                commandList->OMSetRenderTargets(1, &upHandle, false, nullptr);
                float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                commandList->ClearRenderTargetView(upHandle, clearColor, 0, nullptr);
            }

            DrawSinglePass(commandList, Mode::DualKawaseBlur, prevSource, upHandle, (isFinalUp && isLastBatch), upPso);
            
            if (!isFinalUp) {
                DirectXUtils::TransitionBarrier(commandList, kwTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            } else if (!isLastBatch) {
                DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                pingPongIdx++;
            }
            prevSource = kwTex;
        }

        // Viewportを元の画面サイズに戻す
        D3D12_VIEWPORT fullViewport{};
        fullViewport.Width = (FLOAT)dxCommon_->GetClientWidth();
        fullViewport.Height = (FLOAT)dxCommon_->GetClientHeight();
        fullViewport.MinDepth = 0.0f;
        fullViewport.MaxDepth = 1.0f;
        commandList->RSSetViewports(1, &fullViewport);

        D3D12_RECT fullScissorRect{};
        fullScissorRect.right = dxCommon_->GetClientWidth();
        fullScissorRect.bottom = dxCommon_->GetClientHeight();
        commandList->RSSetScissorRects(1, &fullScissorRect);

        currentSource = nextTarget;
        modeIdx++;
      }
      // 2) 統合バッチ
      else {
        std::vector<Mode> batch;
        size_t lookAhead = modeIdx;
        while (lookAhead < activeModes_.size() && 
               activeModes_[lookAhead] != Mode::Bloom && 
               activeModes_[lookAhead] != Mode::Smoothing && 
               activeModes_[lookAhead] != Mode::GaussianFilter && 
               activeModes_[lookAhead] != Mode::DualKawaseBlur && 
               batch.size() < 16) {
            
            bool needsSeparate = RequiresSeparatePass(activeModes_[lookAhead]);

            // すでにバッチに他のエフェクト（色調補正等）が入っている状態で
            // 独立パス要求のエフェクトが来た場合、一度バッチを区切ってテクスチャに書き込む
            if (!batch.empty() && needsSeparate) {
                break;
            }

            batch.push_back(activeModes_[lookAhead]);
            lookAhead++;

            // 独立パス要求のエフェクトを追加した直後もバッチを区切る（単独パス化）
            if (needsSeparate) {
                break;
            }
        }

        isLastBatch = (lookAhead == activeModes_.size());
        RenderTexture* nextTarget = isLastBatch ? nullptr : workspace.workTextures[pingPongIdx % 2];
        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle = isLastBatch ? rtvHandle : nextTarget->GetRtvHandle();

        if (!isLastBatch) {
            DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        combinedParams_.effectCount = (int32_t)batch.size();
        for (int i = 0; i < (int)batch.size(); ++i) {
          combinedParams_.effects[i] = (int32_t)batch[i];
        }
        if (mappedCombined_) { *mappedCombined_ = combinedParams_; }

        commandList->OMSetRenderTargets(1, &targetHandle, false, nullptr);
        float clearColor[] = { 0, 0, 0, 1 };
        commandList->ClearRenderTargetView(targetHandle, clearColor, 0, nullptr);

        commandList->SetPipelineState(isLastBatch ? finalCombinedPSO_.Get() : combinedPSO_.Get());
        commandList->SetGraphicsRootSignature(rootSig_);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Bindless パラメータの更新
        mappedBindless_[bindlessBufferOffset_].mainTextureIndex = currentSource ? currentSource->GetSrvIndex() : 0;
        // wait, we need to handle dissolveNoise and depthSrv for combined pass
        uint32_t extraIdx = depthSrvIndex_;
        for (int i = 0; i < (int)batch.size(); ++i) {
            if (UsesDepthBuffer(batch[i])) {
                extraIdx = depthSrvIndex_;
            } else if (batch[i] == Mode::Dissolve) {
                int noiseIdx = (dissolveParams_.noiseType <= 0) ? 0 : 1;
                extraIdx = dissolveNoiseIndex_[noiseIdx];
            }
        }
        mappedBindless_[bindlessBufferOffset_].extraTextureIndex = extraIdx;
        mappedBindless_[bindlessBufferOffset_].maskTextureIndex = dxCommon_->GetEngine()->GetEffectMaskTexture()->GetSrvIndex();
        
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
        bindlessBufferOffset_++;
        
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, combinedCB_->GetGPUVirtualAddress());
        commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::CustomEffectParams, customEffectParamsCB_->GetGPUVirtualAddress());
        commandList->DrawInstanced(3, 1, 0, 0);

        if (!isLastBatch) {
          DirectXUtils::TransitionBarrier(commandList, nextTarget->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
          pingPongIdx++;
        }
        currentSource = nextTarget;
        modeIdx = lookAhead;
      }
    }
  } else {
    // グローバルエフェクト(activeModes_)が無い場合でも、個別オブジェクトエフェクト(マスク)のために
    // 統合シェーダー(PostProcess.PS.hlsl)を最低1回は最終パスとして回す必要がある
    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    commandList->SetPipelineState(finalCombinedPSO_.Get());
    commandList->SetGraphicsRootSignature(rootSig_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    mappedBindless_[bindlessBufferOffset_].mainTextureIndex = srcTexture ? srcTexture->GetSrvIndex() : 0;
    mappedBindless_[bindlessBufferOffset_].extraTextureIndex = depthSrvIndex_;
    mappedBindless_[bindlessBufferOffset_].maskTextureIndex = dxCommon_->GetEngine()->GetEffectMaskTexture()->GetSrvIndex();

    commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
    bindlessBufferOffset_++;

    commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, combinedCB_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::CustomEffectParams, customEffectParamsCB_->GetGPUVirtualAddress());
    commandList->DrawInstanced(3, 1, 0, 0);
  }
}

void PostProcessManager::DrawSinglePass(ID3D12GraphicsCommandList *commandList,
                                        Mode mode, RenderTexture *srcTexture,
                                        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                                        bool isFinalPass,
                                        ID3D12PipelineState* psoOverride) {
  uint32_t modeIdx = static_cast<uint32_t>(mode);
  if (!psoOverride && (modeIdx >= psos_.size() || !psos_[modeIdx]))
    return;

  // レンダーターゲットの設定
  commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

  // 描画前にターゲットをクリアする（Shaderで discard された箇所が背景色として残るように）
  float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  if (mode == Mode::Dissolve) {
    clearColor[0] = dissolveParams_.backgroundColor.x;
    clearColor[1] = dissolveParams_.backgroundColor.y;
    clearColor[2] = dissolveParams_.backgroundColor.z;
    clearColor[3] = dissolveParams_.backgroundColor.w;
  }
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  // PSOとルートシグネチャの設定
  ID3D12PipelineState* pso = psoOverride;
  if (!pso) {
      pso = isFinalPass ? finalPsos_[modeIdx].Get() : psos_[modeIdx].Get();
  }
  if (!pso) return;

  commandList->SetPipelineState(pso);
  commandList->SetGraphicsRootSignature(rootSig_);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // ソーステクスチャのバインド (Root2 -> t0)

  
  // -------------------------------------------------------------
  // Bindless パラメータの更新
  mappedBindless_[bindlessBufferOffset_].mainTextureIndex = srcTexture ? srcTexture->GetSrvIndex() : 0;
  
  uint32_t extraIdx = 0;
  if (mode == Mode::DepthBasedOutline) {
      extraIdx = depthSrvIndex_;
  } else if (mode == Mode::Dissolve) {
      int noiseIdx = (dissolveParams_.noiseType <= 0) ? 0 : 1;
      extraIdx = dissolveNoiseIndex_[noiseIdx];
  }
  mappedBindless_[bindlessBufferOffset_].extraTextureIndex = extraIdx;
  
  // マスクテクスチャのインデックスを渡す
  mappedBindless_[bindlessBufferOffset_].maskTextureIndex = dxCommon_->GetEngine()->GetEffectMaskTexture()->GetSrvIndex();
  
  commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::LightCommon, bindlessCB_->GetGPUVirtualAddress() + bindlessBufferOffset_ * sizeof(BindlessParams));
  bindlessBufferOffset_++;
  // -------------------------------------------------------------

  // 定数バッファのバインド (Root0 -> b0)
  D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = 0;
  switch (mode) {
  case Mode::Vignette:
    cbvAddress = vignetteCB_->GetGPUVirtualAddress();
    break;
  case Mode::Smoothing:
    cbvAddress = smoothingCB_->GetGPUVirtualAddress();
    break;
  case Mode::GaussianFilter:
    cbvAddress = gaussianCB_->GetGPUVirtualAddress();
    break;
  case Mode::RadialBlur:
    cbvAddress = radialBlurCB_->GetGPUVirtualAddress();
    break;
  case Mode::DepthBasedOutline:
    cbvAddress = outlineCB_->GetGPUVirtualAddress();
    break;
  case Mode::Dissolve:
    cbvAddress = dissolveCB_->GetGPUVirtualAddress();
    break;
  case Mode::Noise:
    cbvAddress = noiseCB_->GetGPUVirtualAddress();
    break;
  case Mode::HSV:
    cbvAddress = hsvCB_->GetGPUVirtualAddress();
    break;
  case Mode::ToneMapping:
    cbvAddress = toneMappingCB_->GetGPUVirtualAddress();
    break;
  case Mode::Fade:
    cbvAddress = fadeCB_->GetGPUVirtualAddress();
    break;
  case Mode::Slide:
    cbvAddress = slideCB_->GetGPUVirtualAddress();
    break;
  case Mode::Bloom:
    cbvAddress = bloomCB_->GetGPUVirtualAddress();
    break;
  case Mode::Glitch:
    cbvAddress = glitchCB_->GetGPUVirtualAddress();
    break;
  case Mode::DualKawaseBlur:
    cbvAddress = dualKawaseCB_->GetGPUVirtualAddress();
    break;
  default:
    break;
  }
  if (cbvAddress != 0) {
    commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, cbvAddress);
  }

  // 追加のリソース（深度、ノイズテクスチャ）のバインド (Root12 -> t1)
  if (mode == Mode::DepthBasedOutline) {
  } else if (mode == Mode::Dissolve) {
    int32_t noiseIdx = (std::max)(int32_t(0), (std::min)(int32_t(1), dissolveParams_.noiseType));
  }

  // 描画実行 (3頂点インデックスなし)
  commandList->DrawInstanced(3, 1, 0, 0);
}

void PostProcessManager::CreatePSOs() {
  auto* shaderManager = dxCommon_->GetShaderManager();
  
  // --- シェーダコンパイル設定 ---
  ShaderCompileOptions options;
#if defined(_DEBUG) || defined(DEVELOPMENT) || defined(EditorMode)
  options.isDebug = true;
#endif

  auto vsBlob = shaderManager->GetOrCompile(L"Fullscreen.VS.hlsl", options);

  struct ShaderPath {
    Mode mode;
    std::wstring path;
  };
  std::vector<ShaderPath> shaders = {
      {Mode::None, L"CopyImage.PS.hlsl"},
      {Mode::Grayscale, L"Grayscale.PS.hlsl"},
      {Mode::Sepia, L"Sepia.PS.hlsl"},
      {Mode::Vignette, L"Vignette.PS.hlsl"},
      {Mode::Smoothing, L"Smoothing.PS.hlsl"},
      {Mode::GaussianFilter, L"GaussianFilter.PS.hlsl"},
      {Mode::DepthBasedOutline, L"DepthBasedOutline.PS.hlsl"},
      {Mode::RadialBlur, L"RadialBlur.PS.hlsl"},
      {Mode::Dissolve, L"Dissolve.PS.hlsl"},
      {Mode::Noise, L"Noise.PS.hlsl"},
      {Mode::HSV, L"HSV.PS.hlsl"},
      {Mode::ToneMapping, L"ToneMapping.PS.hlsl"},
      {Mode::Fade, L"Fade.PS.hlsl"},
      {Mode::Slide, L"Slide.PS.hlsl"},
      {Mode::Bloom, L"CopyImage.PS.hlsl"},
      {Mode::Glitch, L"Glitch.PS.hlsl"},
  };

  for (const auto &s : shaders) {
    auto psBlob = shaderManager->GetOrCompile(s.path, options);
    if (!psBlob)
      continue;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSig_;
    desc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
    desc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

    desc.BlendState.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.DepthStencilState.DepthEnable = FALSE;
    desc.DepthStencilState.StencilEnable = FALSE;
    desc.InputLayout = {nullptr, 0};
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.NumRenderTargets = 1;
    desc.SampleDesc.Count = 1;

    // 中間パス用 (_UNORM)
    desc.RTVFormats[0] = rtvFormat_;
    device_->CreateGraphicsPipelineState(
        &desc, IID_PPV_ARGS(&psos_[static_cast<uint32_t>(s.mode)]));

    // 最終パス用 (_SRGB)
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    device_->CreateGraphicsPipelineState(
        &desc, IID_PPV_ARGS(&finalPsos_[static_cast<uint32_t>(s.mode)]));
  }

  // --- ブルーム用個別 PSO ---
    auto extractPS = shaderManager->GetOrCompile(L"HighLuminanceExtract.PS.hlsl", options);
    auto blurPS = shaderManager->GetOrCompile(L"GaussianBlur.PS.hlsl", options);
    auto combinePS = shaderManager->GetOrCompile(L"BloomCombine.PS.hlsl", options);

    if (extractPS && blurPS && combinePS) {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC bloomDesc{};
      bloomDesc.pRootSignature = rootSig_;
      bloomDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
      bloomDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
      bloomDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
      bloomDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
      bloomDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
      bloomDesc.DepthStencilState.DepthEnable = FALSE;
      bloomDesc.DepthStencilState.StencilEnable = FALSE;
      bloomDesc.InputLayout = {nullptr, 0};
      bloomDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      bloomDesc.NumRenderTargets = 1;
      bloomDesc.RTVFormats[0] = rtvFormat_;
      bloomDesc.SampleDesc.Count = 1;

      bloomDesc.PS = {extractPS->GetBufferPointer(), extractPS->GetBufferSize()};
      device_->CreateGraphicsPipelineState(&bloomDesc, IID_PPV_ARGS(&bloomExtractPSO_));

      bloomDesc.PS = {blurPS->GetBufferPointer(), blurPS->GetBufferSize()};
      device_->CreateGraphicsPipelineState(&bloomDesc, IID_PPV_ARGS(&bloomBlurHPSO_));
      device_->CreateGraphicsPipelineState(&bloomDesc, IID_PPV_ARGS(&bloomBlurVPSO_));

      bloomDesc.PS = {combinePS->GetBufferPointer(), combinePS->GetBufferSize()};
      bloomDesc.RTVFormats[0] = rtvFormat_; // 中間パス用 (_UNORM)
      device_->CreateGraphicsPipelineState(&bloomDesc, IID_PPV_ARGS(&bloomCombinePSO_));

      bloomDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 最終パス用 (_SRGB)
      device_->CreateGraphicsPipelineState(&bloomDesc, IID_PPV_ARGS(&finalBloomCombinePSO_));
    }

    // --- 統合ポストプロセス用 PSO ---
    auto combinedPS = shaderManager->GetOrCompile(L"PostProcess.PS.hlsl", options);
    if (combinedPS) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC combinedDesc{}; // ベースを流用
        combinedDesc.pRootSignature = rootSig_;
        combinedDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        combinedDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        combinedDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        combinedDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        combinedDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        combinedDesc.DepthStencilState.DepthEnable = FALSE;
        combinedDesc.DepthStencilState.StencilEnable = FALSE;
        combinedDesc.InputLayout = {nullptr, 0};
        combinedDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        combinedDesc.NumRenderTargets = 1;
        combinedDesc.SampleDesc.Count = 1;
        combinedDesc.PS = {combinedPS->GetBufferPointer(), combinedPS->GetBufferSize()};
        
        // 中間パス用
        combinedDesc.RTVFormats[0] = rtvFormat_;
        device_->CreateGraphicsPipelineState(&combinedDesc, IID_PPV_ARGS(&combinedPSO_));
        // 最終パス用
        combinedDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        device_->CreateGraphicsPipelineState(&combinedDesc, IID_PPV_ARGS(&finalCombinedPSO_));
    }

    // --- 分離可能フィルタ用 PSO ---
    auto boxBlurPS = shaderManager->GetOrCompile(L"BoxBlur.PS.hlsl", options);
    auto gaussianPS = shaderManager->GetOrCompile(L"GaussianFilter.PS.hlsl", options);
    
    if (boxBlurPS && gaussianPS) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC sepDesc{};
        sepDesc.pRootSignature = rootSig_;
        sepDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        sepDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        sepDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        sepDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        sepDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        sepDesc.DepthStencilState.DepthEnable = FALSE;
        sepDesc.DepthStencilState.StencilEnable = FALSE;
        sepDesc.InputLayout = {nullptr, 0};
        sepDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        sepDesc.NumRenderTargets = 1;
        sepDesc.SampleDesc.Count = 1;

        // BoxBlur PSO
        sepDesc.RTVFormats[0] = rtvFormat_;
        sepDesc.PS = {boxBlurPS->GetBufferPointer(), boxBlurPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&sepDesc, IID_PPV_ARGS(&smoothingBlurPSO_));
        sepDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        device_->CreateGraphicsPipelineState(&sepDesc, IID_PPV_ARGS(&finalSmoothingBlurPSO_));

        // GaussianFilter PSO
        sepDesc.RTVFormats[0] = rtvFormat_;
        sepDesc.PS = {gaussianPS->GetBufferPointer(), gaussianPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&sepDesc, IID_PPV_ARGS(&gaussianBlurPSO_));
        sepDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        device_->CreateGraphicsPipelineState(&sepDesc, IID_PPV_ARGS(&finalGaussianBlurPSO_));
    }

    // --- Dual Kawase Blur 用 PSO ---
    auto kawaseDownPS = shaderManager->GetOrCompile(L"DualKawaseDownsample.PS.hlsl", options);
    auto kawaseUpPS = shaderManager->GetOrCompile(L"DualKawaseUpsample.PS.hlsl", options);
    if (kawaseDownPS && kawaseUpPS) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC kwDesc{};
        kwDesc.pRootSignature = rootSig_;
        kwDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        kwDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        kwDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        kwDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        kwDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        kwDesc.DepthStencilState.DepthEnable = FALSE;
        kwDesc.DepthStencilState.StencilEnable = FALSE;
        kwDesc.InputLayout = {nullptr, 0};
        kwDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        kwDesc.NumRenderTargets = 1;
        kwDesc.SampleDesc.Count = 1;

        kwDesc.RTVFormats[0] = rtvFormat_;
        kwDesc.PS = {kawaseDownPS->GetBufferPointer(), kawaseDownPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&kwDesc, IID_PPV_ARGS(&dualKawaseDownsamplePSO_));

        kwDesc.PS = {kawaseUpPS->GetBufferPointer(), kawaseUpPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&kwDesc, IID_PPV_ARGS(&dualKawaseUpsamplePSO_));

        kwDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        device_->CreateGraphicsPipelineState(&kwDesc, IID_PPV_ARGS(&finalDualKawaseUpsamplePSO_));
    }

    // --- Light Shafts 用 PSO ---
    auto lsExtractPS = shaderManager->GetOrCompile(L"LightShaftsExtract.PS.hlsl", options);
    auto lsRadialBlurPS = shaderManager->GetOrCompile(L"LightShaftsRadialBlur.PS.hlsl", options);
    auto lsCombinePS = shaderManager->GetOrCompile(L"LightShaftsCombine.PS.hlsl", options);

    if (lsExtractPS && lsRadialBlurPS && lsCombinePS) {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC lsDesc{};
        lsDesc.pRootSignature = rootSig_;
        lsDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        lsDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        lsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        lsDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        lsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        lsDesc.DepthStencilState.DepthEnable = FALSE;
        lsDesc.DepthStencilState.StencilEnable = FALSE;
        lsDesc.InputLayout = {nullptr, 0};
        lsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        lsDesc.NumRenderTargets = 1;
        lsDesc.RTVFormats[0] = rtvFormat_;
        lsDesc.SampleDesc.Count = 1;

        lsDesc.PS = {lsExtractPS->GetBufferPointer(), lsExtractPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&lsDesc, IID_PPV_ARGS(&lsExtractPSO_));

        lsDesc.PS = {lsRadialBlurPS->GetBufferPointer(), lsRadialBlurPS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&lsDesc, IID_PPV_ARGS(&lsRadialBlurPSO_));

        lsDesc.PS = {lsCombinePS->GetBufferPointer(), lsCombinePS->GetBufferSize()};
        device_->CreateGraphicsPipelineState(&lsDesc, IID_PPV_ARGS(&lsCombinePSO_));

        lsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        device_->CreateGraphicsPipelineState(&lsDesc, IID_PPV_ARGS(&finalLsCombinePSO_));
    }
}

void PostProcessManager::CreateConstantBuffers() {
    combinedCB_ = CreateBuffer(sizeof(CombinedParams));
  bindlessCB_ = CreateBuffer(256 * 64);
  customEffectParamsCB_ = CreateBuffer(sizeof(CustomEffectParams) * kMaxCustomEffectParams);

  bindlessCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBindless_));
  combinedCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedCombined_));
  customEffectParamsCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedCustomEffectParams_));

  noiseCB_ = CreateBuffer(sizeof(NoiseParams));
  noiseCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedNoise_));

  vignetteCB_ = CreateBuffer(sizeof(VignetteParams));
  vignetteCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedVignette_));

  smoothingCB_ = CreateBuffer(sizeof(SmoothingParams));
  smoothingCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedSmoothing_));

  gaussianCB_ = CreateBuffer(sizeof(GaussianParams));
  gaussianCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedGaussian_));

  radialBlurCB_ = CreateBuffer(sizeof(RadialBlurParams));
  radialBlurCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedRadialBlur_));

  outlineCB_ = CreateBuffer(sizeof(OutlineParams));
  outlineCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedOutline_));

  dissolveCB_ = CreateBuffer(sizeof(DissolveParams));
  dissolveCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedDissolve_));

  hsvCB_ = CreateBuffer(sizeof(HSVParams));
  hsvCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedHsv_));

  toneMappingCB_ = CreateBuffer(sizeof(ToneMappingParams));
  toneMappingCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedToneMapping_));

  fadeCB_ = CreateBuffer(sizeof(FadeParams));
  fadeCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedFade_));

  slideCB_ = CreateBuffer(sizeof(SlideParams));
  slideCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedSlide_));

  bloomCB_ = CreateBuffer(sizeof(BloomParams));
  bloomCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedBloom_));

  glitchCB_ = CreateBuffer(sizeof(GlitchParams));
  glitchCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedGlitch_));

  dualKawaseCB_ = CreateBuffer(sizeof(DualKawaseBlurParams));
  dualKawaseCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedDualKawase_));

  lightShaftsCB_ = CreateBuffer(sizeof(LightShaftsParams));
  lightShaftsCB_->Map(0, nullptr, reinterpret_cast<void **>(&mappedLightShafts_));
}

Microsoft::WRL::ComPtr<ID3D12Resource>
PostProcessManager::CreateBuffer(size_t size) {
  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = (size + 255) & ~255;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  Microsoft::WRL::ComPtr<ID3D12Resource> resource;
  device_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                   IID_PPV_ARGS(&resource));
  return resource;
}
