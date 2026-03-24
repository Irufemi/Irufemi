#include "PostProcessManager.h"
#include "../DirectX/DirectXCommon.h"
#include "../../Core/Utility/Log.h"
#include "../../Core/Math/Geometry/Math.h"
#include <d3d12.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>

void PostProcessManager::Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, DXGI_FORMAT rtvFormat) {
    device_ = device;
    rootSig_ = rootSig;
    rtvFormat_ = rtvFormat;

    CreateConstantBuffers();
    CreatePSOs();
}

namespace {
    void TransitionResource(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) {
        if (stateBefore == stateAfter) return;
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);
    }
}

void PostProcessManager::InitializeBuffers(uint32_t width, uint32_t height, DirectXCommon* dxCommon) {
    if (!dxCommon) return;
    for (int i = 0; i < 2; ++i) {
        if (!workTextures_[i]) {
            workTextures_[i] = std::make_unique<RenderTexture>();
        }
        workTextures_[i]->Initialize(dxCommon, width, height, rtvFormat_, { 0.0f, 0.0f, 0.0f, 1.0f });
        
        // デバッグ用に名前を付ける
        if (workTextures_[i]->GetResource()) {
            std::wstring name = L"PostProcess_Work" + std::to_wstring(i);
            workTextures_[i]->GetResource()->SetName(name.c_str());
        }

        // 生成直後は PIXEL_SHADER_RESOURCE 状態 (DirectXCommon::CreateRenderTextureResource の仕様に合わせる)
        workTextureStates_[i] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}

void PostProcessManager::Update(float totalTime) {
    noiseParams_.time = totalTime;
    if (mappedNoise_) {
        *mappedNoise_ = noiseParams_;
    }
    
    // 他のパラメータも必要に応じてマッピング
    if (mappedVignette_) *mappedVignette_ = vignetteParams_;
    if (mappedSmoothing_) *mappedSmoothing_ = smoothingParams_;
    if (mappedGaussian_) *mappedGaussian_ = gaussianParams_;
    if (mappedRadialBlur_) *mappedRadialBlur_ = radialBlurParams_;
    if (mappedOutline_) *mappedOutline_ = outlineParams_;
    if (mappedDissolve_) *mappedDissolve_ = dissolveParams_;
}

void PostProcessManager::Draw(ID3D12GraphicsCommandList* commandList, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {
    if (activeModes_.empty()) {
        // エフェクトなし（None）の場合は、単に srcTexture を出力へコピー
        DrawSinglePass(commandList, Mode::None, srcTexture, rtvHandle);
        return;
    }

    RenderTexture* currentSource = srcTexture;

    for (size_t i = 0; i < activeModes_.size(); ++i) {
        Mode mode = activeModes_[i];
        bool isLast = (i == activeModes_.size() - 1);

        D3D12_CPU_DESCRIPTOR_HANDLE targetHandle;
        RenderTexture* nextTargetTexture = nullptr;
        int nextTargetIdx = -1;

        if (isLast) {
            targetHandle = rtvHandle;
        } else {
            nextTargetIdx = static_cast<int>(i % 2);
            nextTargetTexture = workTextures_[nextTargetIdx].get();
            targetHandle = nextTargetTexture->GetRtvHandle();
            
            // 現在の状態から RENDER_TARGET へ遷移
            TransitionResource(commandList, nextTargetTexture->GetResource(), workTextureStates_[nextTargetIdx], D3D12_RESOURCE_STATE_RENDER_TARGET);
            workTextureStates_[nextTargetIdx] = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        DrawSinglePass(commandList, mode, currentSource, targetHandle);

        if (!isLast) {
            // 次のパスで読み込むために PIXEL_SHADER_RESOURCE へ遷移
            TransitionResource(commandList, nextTargetTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            workTextureStates_[nextTargetIdx] = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
            currentSource = nextTargetTexture;
        }
    }
}

void PostProcessManager::DrawSinglePass(ID3D12GraphicsCommandList* commandList, Mode mode, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle) {
    uint32_t modeIdx = static_cast<uint32_t>(mode);
    if (modeIdx >= psos_.size() || !psos_[modeIdx]) return;

    // レンダーターゲットの設定
    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
    
    // PSOとルートシグネチャの設定
    commandList->SetPipelineState(psos_[modeIdx].Get());
    commandList->SetGraphicsRootSignature(rootSig_);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // ソーステクスチャのバインド (Root2 -> t0)
    commandList->SetGraphicsRootDescriptorTable(2, srcTexture->GetSrvHandleGPU());

    // 定数バッファのバインド (Root0 -> b0)
    D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = 0;
    switch (mode) {
    case Mode::Vignette: cbvAddress = vignetteCB_->GetGPUVirtualAddress(); break;
    case Mode::Smoothing: cbvAddress = smoothingCB_->GetGPUVirtualAddress(); break;
    case Mode::GaussianFilter: cbvAddress = gaussianCB_->GetGPUVirtualAddress(); break;
    case Mode::RadialBlur: cbvAddress = radialBlurCB_->GetGPUVirtualAddress(); break;
    case Mode::DepthBasedOutline: cbvAddress = outlineCB_->GetGPUVirtualAddress(); break;
    case Mode::Dissolve: cbvAddress = dissolveCB_->GetGPUVirtualAddress(); break;
    case Mode::Noise: cbvAddress = noiseCB_->GetGPUVirtualAddress(); break;
    default: break;
    }
    if (cbvAddress != 0) {
        commandList->SetGraphicsRootConstantBufferView(0, cbvAddress);
    }

    // 追加のリソース（深度、ノイズテクスチャ）のバインド (Root12 -> t1)
    if (mode == Mode::DepthBasedOutline) {
        commandList->SetGraphicsRootDescriptorTable(12, depthSrvHandle_);
    } else if (mode == Mode::Dissolve) {
        int noiseIdx = (std::max)(0, (std::min)(1, dissolveParams_.noiseType));
        commandList->SetGraphicsRootDescriptorTable(12, dissolveNoiseHandle_[noiseIdx]);
    }

    // 描画実行 (3頂点インデックスなし)
    commandList->DrawInstanced(3, 1, 0, 0);
}

void PostProcessManager::CreatePSOs() {
    // DXCの初期化 (一時的にローカルで)
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    dxcUtils->CreateDefaultIncludeHandler(&includeHandler);

    // ログ出力用ダミー (本当は IrufemiEngine から渡すべきだが、今は std::cout で代用するか Log を使う)
    std::ostream& logStream = std::cout; // 仮

    auto Compile = [&](const std::wstring& path, const wchar_t* profile) {
        return DirectXCommon::CompileShader(path, profile, dxcUtils, dxcCompiler, includeHandler, logStream);
    };

    auto vsBlob = Compile(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
    
    struct ShaderPath {
        Mode mode;
        std::wstring path;
    };
    std::vector<ShaderPath> shaders = {
        { Mode::None, L"resources/shaders/CopyImage.PS.hlsl" },
        { Mode::Grayscale, L"resources/shaders/Grayscale.PS.hlsl" },
        { Mode::Sepia, L"resources/shaders/Sepia.PS.hlsl" },
        { Mode::Vignette, L"resources/shaders/Vignette.PS.hlsl" },
        { Mode::Smoothing, L"resources/shaders/Smoothing.PS.hlsl" },
        { Mode::GaussianFilter, L"resources/shaders/GaussianFilter.PS.hlsl" },
        { Mode::DepthBasedOutline, L"resources/shaders/DepthBasedOutline.PS.hlsl" },
        { Mode::RadialBlur, L"resources/shaders/RadialBlur.PS.hlsl" },
        { Mode::Dissolve, L"resources/shaders/Dissolve.PS.hlsl" },
        { Mode::Noise, L"resources/shaders/Noise.PS.hlsl" },
    };

    for (const auto& s : shaders) {
        auto psBlob = Compile(s.path, L"ps_6_0");
        if (!psBlob) continue;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rootSig_;
        desc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
        desc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
        
        desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.InputLayout = { nullptr, 0 };
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = rtvFormat_;
        desc.SampleDesc.Count = 1;

        device_->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&psos_[static_cast<uint32_t>(s.mode)]));
    }
}

void PostProcessManager::CreateConstantBuffers() {
    noiseCB_ = CreateBuffer(sizeof(NoiseParams));
    noiseCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedNoise_));

    vignetteCB_ = CreateBuffer(sizeof(VignetteParams));
    vignetteCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVignette_));

    smoothingCB_ = CreateBuffer(sizeof(SmoothingParams));
    smoothingCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedSmoothing_));

    gaussianCB_ = CreateBuffer(sizeof(GaussianParams));
    gaussianCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedGaussian_));

    radialBlurCB_ = CreateBuffer(sizeof(RadialBlurParams));
    radialBlurCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedRadialBlur_));

    outlineCB_ = CreateBuffer(sizeof(OutlineParams));
    outlineCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedOutline_));

    dissolveCB_ = CreateBuffer(sizeof(DissolveParams));
    dissolveCB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedDissolve_));
}

Microsoft::WRL::ComPtr<ID3D12Resource> PostProcessManager::CreateBuffer(size_t size) {
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
    device_->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
    return resource;
}
