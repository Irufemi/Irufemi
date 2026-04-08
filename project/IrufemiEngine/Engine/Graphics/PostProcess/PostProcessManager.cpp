/**
 * @file PostProcessManager.cpp
 * @brief ポストプロセス（マルチパス描画）の管理実装クラス
 */
#include "PostProcessManager.h"
#include "../DirectX/DirectXCommon.h"
#include <algorithm>
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h>
#include <iostream>
#include <string>
#include <wrl/client.h>

void PostProcessManager::Initialize(DirectXCommon* dxCommon,
                                    DXGI_FORMAT rtvFormat) {
  dxCommon_ = dxCommon;
  device_ = dxCommon->GetDevice();
  rootSig_ = dxCommon->GetRootSignature();
  rtvFormat_ = rtvFormat;

  CreateConstantBuffers();
  CreatePSOs();
}

namespace {
void TransitionResource(ID3D12GraphicsCommandList *commandList,
                        ID3D12Resource *resource,
                        D3D12_RESOURCE_STATES stateBefore,
                        D3D12_RESOURCE_STATES stateAfter) {
  if (stateBefore == stateAfter)
    return;
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.StateBefore = stateBefore;
  barrier.Transition.StateAfter = stateAfter;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  commandList->ResourceBarrier(1, &barrier);
}
} // namespace

void PostProcessManager::InitializeBuffers(uint32_t width, uint32_t height,
                                           DirectXCommon *dxCommon) {
  if (!dxCommon)
    return;
  for (int i = 0; i < 2; ++i) {
    if (!workTextures_[i]) {
      workTextures_[i] = std::make_unique<RenderTexture>();
    }
    workTextures_[i]->Initialize(dxCommon, width, height, rtvFormat_,
                                 {0.0f, 0.0f, 0.0f, 1.0f});

    // デバッグ用に名前を付ける
    if (workTextures_[i]->GetResource()) {
      std::wstring name = L"PostProcess_Work" + std::to_wstring(i);
      workTextures_[i]->GetResource()->SetName(name.c_str());
    }

    // 生成直後は PIXEL_SHADER_RESOURCE 状態
    // (DirectXCommon::CreateRenderTextureResource の仕様に合わせる)
    workTextureStates_[i] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  }

  // 最終結果保持用テクスチャの初期化
  if (!resultTexture_) {
    resultTexture_ = std::make_unique<RenderTexture>();
  }
  resultTexture_->Initialize(dxCommon, width, height, rtvFormat_,
                             {0.0f, 0.0f, 0.0f, 1.0f});
  if (resultTexture_->GetResource()) {
    resultTexture_->GetResource()->SetName(L"PostProcess_Result");
  }
  resultTextureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void PostProcessManager::Update(float totalTime) {
  noiseParams_.time = totalTime;
  if (mappedNoise_) {
    *mappedNoise_ = noiseParams_;
  }

  // 他のパラメータも必要に応じてマッピング
  if (mappedVignette_)
    *mappedVignette_ = vignetteParams_;
  if (mappedSmoothing_)
    *mappedSmoothing_ = smoothingParams_;
  if (mappedGaussian_)
    *mappedGaussian_ = gaussianParams_;
  if (mappedRadialBlur_)
    *mappedRadialBlur_ = radialBlurParams_;
  if (mappedOutline_)
    *mappedOutline_ = outlineParams_;
  if (mappedDissolve_)
    *mappedDissolve_ = dissolveParams_;
  if (mappedHsv_)
    *mappedHsv_ = hsvParams_;
  if (mappedToneMapping_)
    *mappedToneMapping_ = toneMappingParams_;
  if (mappedFade_)
    *mappedFade_ = fadeParams_;
  if (mappedSlide_)
    *mappedSlide_ = slideParams_;
}

void PostProcessManager::Draw(ID3D12GraphicsCommandList *commandList,
                              RenderTexture *srcTexture,
                              D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {
  RenderTexture *currentSource = srcTexture;

  // 1) ポストプロセスをテクスチャ間で実行し、最終結果を resultTexture_ に入れる
  if (!activeModes_.empty()) {
    for (size_t i = 0; i < activeModes_.size(); ++i) {
      Mode mode = activeModes_[i];
      bool isLast = (i == activeModes_.size() - 1);

      D3D12_CPU_DESCRIPTOR_HANDLE targetHandle;
      RenderTexture *nextTargetTexture = nullptr;
      int nextTargetIdx = -1;

      if (isLast) {
        // 最終パスは resultTexture_ に出力
        nextTargetTexture = resultTexture_.get();
        targetHandle = nextTargetTexture->GetRtvHandle();
        TransitionResource(commandList, nextTargetTexture->GetResource(),
                           resultTextureState_,
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        resultTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
      } else {
        nextTargetIdx = static_cast<int>(i % 2);
        nextTargetTexture = workTextures_[nextTargetIdx].get();
        targetHandle = nextTargetTexture->GetRtvHandle();

        TransitionResource(commandList, nextTargetTexture->GetResource(),
                           workTextureStates_[nextTargetIdx],
                           D3D12_RESOURCE_STATE_RENDER_TARGET);
        workTextureStates_[nextTargetIdx] = D3D12_RESOURCE_STATE_RENDER_TARGET;
      }

      DrawSinglePass(commandList, mode, currentSource, targetHandle);

      // 状態遷移
      if (isLast) {
        TransitionResource(commandList, nextTargetTexture->GetResource(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        resultTextureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        currentSource = nextTargetTexture;
      } else {
        TransitionResource(commandList, nextTargetTexture->GetResource(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        workTextureStates_[nextTargetIdx] =
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        currentSource = nextTargetTexture;
      }
    }
  } else {
    // エフェクトなしの場合は srcTexture を resultTexture_ にコピー
    TransitionResource(commandList, resultTexture_->GetResource(),
                       resultTextureState_, D3D12_RESOURCE_STATE_RENDER_TARGET);
    DrawSinglePass(commandList, Mode::None, srcTexture,
                   resultTexture_->GetRtvHandle());
    TransitionResource(commandList, resultTexture_->GetResource(),
                       D3D12_RESOURCE_STATE_RENDER_TARGET,
                       D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    resultTextureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    currentSource = resultTexture_.get();
  }

  // 2) 最終結果をバックバッファ（提供された rtvHandle）に転送
  // これにより、既存の全画面表示も維持される
  DrawSinglePass(commandList, Mode::None, currentSource, rtvHandle, true);
}

void PostProcessManager::DrawSinglePass(ID3D12GraphicsCommandList *commandList,
                                        Mode mode, RenderTexture *srcTexture,
                                        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
                                        bool isFinalPass) {
  uint32_t modeIdx = static_cast<uint32_t>(mode);
  if (modeIdx >= psos_.size() || !psos_[modeIdx])
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
  ID3D12PipelineState* pso = isFinalPass ? finalPsos_[modeIdx].Get() : psos_[modeIdx].Get();
  if (!pso) return;

  commandList->SetPipelineState(pso);
  commandList->SetGraphicsRootSignature(rootSig_);
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // ソーステクスチャのバインド (Root2 -> t0)
  commandList->SetGraphicsRootDescriptorTable((UINT)RootSlot::Texture, srcTexture->GetSrvHandleGPU());

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
  default:
    break;
  }
  if (cbvAddress != 0) {
    commandList->SetGraphicsRootConstantBufferView((UINT)RootSlot::Material, cbvAddress);
  }

  // 追加のリソース（深度、ノイズテクスチャ）のバインド (Root12 -> t1)
  if (mode == Mode::DepthBasedOutline) {
    commandList->SetGraphicsRootDescriptorTable((UINT)RootSlot::EnvMap, depthSrvHandle_);
  } else if (mode == Mode::Dissolve) {
    int noiseIdx = (std::max)(0, (std::min)(1, dissolveParams_.noiseType));
    commandList->SetGraphicsRootDescriptorTable((UINT)RootSlot::EnvMap,
                                                dissolveNoiseHandle_[noiseIdx]);
  }

  // 描画実行 (3頂点インデックスなし)
  commandList->DrawInstanced(3, 1, 0, 0);
}

void PostProcessManager::CreatePSOs() {
  auto* shaderCompiler = dxCommon_->GetShaderCompiler();
  std::ostream &logStream = std::cout; // 仮

  auto vsBlob = shaderCompiler->Compile(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0", logStream);

  struct ShaderPath {
    Mode mode;
    std::wstring path;
  };
  std::vector<ShaderPath> shaders = {
      {Mode::None, L"resources/shaders/CopyImage.PS.hlsl"},
      {Mode::Grayscale, L"resources/shaders/Grayscale.PS.hlsl"},
      {Mode::Sepia, L"resources/shaders/Sepia.PS.hlsl"},
      {Mode::Vignette, L"resources/shaders/Vignette.PS.hlsl"},
      {Mode::Smoothing, L"resources/shaders/Smoothing.PS.hlsl"},
      {Mode::GaussianFilter, L"resources/shaders/GaussianFilter.PS.hlsl"},
      {Mode::DepthBasedOutline, L"resources/shaders/DepthBasedOutline.PS.hlsl"},
      {Mode::RadialBlur, L"resources/shaders/RadialBlur.PS.hlsl"},
      {Mode::Dissolve, L"resources/shaders/Dissolve.PS.hlsl"},
      {Mode::Noise, L"resources/shaders/Noise.PS.hlsl"},
      {Mode::HSV, L"resources/shaders/HSV.PS.hlsl"},
      {Mode::ToneMapping, L"resources/shaders/ToneMapping.PS.hlsl"},
      {Mode::Fade, L"resources/shaders/Fade.PS.hlsl"},
      {Mode::Slide, L"resources/shaders/Slide.PS.hlsl"},
  };

  for (const auto &s : shaders) {
    auto psBlob = shaderCompiler->Compile(s.path, L"ps_6_0", logStream);
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
}

void PostProcessManager::CreateConstantBuffers() {
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
