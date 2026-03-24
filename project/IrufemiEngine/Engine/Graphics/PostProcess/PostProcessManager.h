#pragma once

#include "../../Core/Math/Vector2.h"
#include "../../Core/Math/Vector4.h"
#include "../../Core/Math/Matrix4x4.h"
#include "../DirectX/RenderTexture.h"
#include <d3d12.h>
#include <wrl/client.h>
#ifndef __IInspectable_INTERFACE_DEFINED__
// ComPtr の一部のテンプレート展開で IInspectable が必要になる場合がある
typedef struct IInspectable IInspectable;
#endif
#include <memory>
#include <vector>
#include <array>
#include <cstdint>

enum class PostProcessMode {
    None,
    Grayscale,
    Sepia,
    Vignette,
    Smoothing,
    GaussianFilter,
    DepthBasedOutline,
    RadialBlur,
    Dissolve,
    Noise,
};

class DirectXCommon;

class PostProcessManager {
public:
    using Mode = PostProcessMode;


    struct NoiseParams {
        float intensity = 0.5f;
        float time = 0.0f;
    };

    struct VignetteParams {
        float scale = 16.0f;
        float power = 0.8f;
    };

    struct SmoothingParams {
        int32_t kernelSize = 3;
    };

    struct GaussianParams {
        float sigma = 2.0f;
        int32_t kernelSize = 3;
    };

    struct RadialBlurParams {
        Vector2 center = { 0.5f, 0.5f };
        float blurWidth = 0.01f;
        int32_t numSamples = 10;
    };

    struct OutlineParams {
        Matrix4x4 projectionInverse;
    };

    struct DissolveParams {
        Vector4 edgeColor = { 1.0f, 0.4f, 0.3f, 1.0f };
        float threshold = 0.0f;
        float edgeRange = 0.03f;
        int32_t noiseType = 0;
    };

public:
    void Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig, DXGI_FORMAT rtvFormat);
    void InitializeBuffers(uint32_t width, uint32_t height, DirectXCommon* dxCommon);
    void Update(float totalTime);
    void Draw(ID3D12GraphicsCommandList* commandList, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);

    // Getters & Setters
    const std::vector<Mode>& GetActiveModes() const { return activeModes_; }
    void AddActiveMode(Mode mode) { activeModes_.push_back(mode); }
    void ClearActiveModes() { activeModes_.clear(); }
    void SetActiveModes(const std::vector<Mode>& modes) { activeModes_ = modes; }
    
    // 互換性のための単一セット (既存コード破壊回避)
    void SetMode(Mode mode) { 
        activeModes_.clear(); 
        if (mode != Mode::None) activeModes_.push_back(mode); 
    }
    Mode GetMode() const { return activeModes_.empty() ? Mode::None : activeModes_.front(); }
    
    NoiseParams& GetNoiseParams() { return noiseParams_; }
    VignetteParams& GetVignetteParams() { return vignetteParams_; }
    SmoothingParams& GetSmoothingParams() { return smoothingParams_; }
    GaussianParams& GetGaussianParams() { return gaussianParams_; }
    RadialBlurParams& GetRadialBlurParams() { return radialBlurParams_; }
    OutlineParams& GetOutlineParams() { return outlineParams_; }
    DissolveParams& GetDissolveParams() { return dissolveParams_; }

    void SetDissolveNoiseHandle(int index, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
        if (index >= 0 && index < 2) dissolveNoiseHandle_[index] = handle;
    }
    
    void SetDepthSrvHandle(D3D12_GPU_DESCRIPTOR_HANDLE handle) { depthSrvHandle_ = handle; }

private:
    void CreatePSOs();
    void CreateConstantBuffers();
    void DrawSinglePass(ID3D12GraphicsCommandList* commandList, Mode mode, RenderTexture* srcTexture, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(size_t size);

private:
    ID3D12Device* device_ = nullptr;
    ID3D12RootSignature* rootSig_ = nullptr;
    DXGI_FORMAT rtvFormat_ = DXGI_FORMAT_UNKNOWN;

    Mode mode_ = Mode::None; // 互換性用（内部では不使用にする）
    std::vector<Mode> activeModes_;
    
    // ピンポンバッファ
    std::array<std::unique_ptr<RenderTexture>, 2> workTextures_;

    // PSOs
    struct PipelineSet {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
    };
    // モードに対応するPSOを保持 (Mode::Noneはnullptr)
    std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 10> psos_;

    // Constant Buffers
    Microsoft::WRL::ComPtr<ID3D12Resource> noiseCB_;
    NoiseParams* mappedNoise_ = nullptr;
    NoiseParams noiseParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vignetteCB_;
    VignetteParams* mappedVignette_ = nullptr;
    VignetteParams vignetteParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> smoothingCB_;
    SmoothingParams* mappedSmoothing_ = nullptr;
    SmoothingParams smoothingParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> gaussianCB_;
    GaussianParams* mappedGaussian_ = nullptr;
    GaussianParams gaussianParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> radialBlurCB_;
    RadialBlurParams* mappedRadialBlur_ = nullptr;
    RadialBlurParams radialBlurParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> outlineCB_;
    OutlineParams* mappedOutline_ = nullptr;
    OutlineParams outlineParams_;

    Microsoft::WRL::ComPtr<ID3D12Resource> dissolveCB_;
    DissolveParams* mappedDissolve_ = nullptr;
    DissolveParams dissolveParams_;

    D3D12_GPU_DESCRIPTOR_HANDLE depthSrvHandle_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, 2> dissolveNoiseHandle_{};

    // 状態追跡用
    std::array<D3D12_RESOURCE_STATES, 2> workTextureStates_ = { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE };
};
