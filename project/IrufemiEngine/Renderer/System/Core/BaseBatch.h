#pragma once

#include "Renderer/System/Core/IRenderable.h"
#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/DescriptorPool.h"
#include "Engine/Graphics/Data/Material.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Core/System/ResourceHandle.h"

class Camera;
class TextureManager;
class DrawManager;
class DescriptorPool;

class BaseBatch : public IRenderable {
public:
    BaseBatch();
    virtual ~BaseBatch();

    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetSrvAllocator(DescriptorPool* alloc) { srvPool_ = alloc; }

    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

    void SetUseGPUCulling(bool use) { useGPUCulling_ = use; }
    bool GetUseGPUCulling() const { return useGPUCulling_; }

    void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
    BlendMode GetBlendMode() const { return blendMode_; }

    void SetDepthWrite(PSOManager::DepthWrite depthWrite) { depthWrite_ = depthWrite; }
    PSOManager::DepthWrite GetDepthWrite() const { return depthWrite_; }

    void SetCullMode(PSOManager::CullMode cullMode) { cullMode_ = cullMode; }
    PSOManager::CullMode GetCullMode() const { return cullMode_; }

    void SetCastShadows(bool cast) { castShadows_ = cast; }
    bool GetCastShadows() const { return castShadows_; }

    void SetCustomPSO(ID3D12PipelineState* pso) { customPSO_ = pso; }
    ID3D12PipelineState* GetCustomPSO() const { return customPSO_; }

    void SetCustomCBVAddress(D3D12_GPU_VIRTUAL_ADDRESS addr) { customCBVAddress_ = addr; }
    D3D12_GPU_VIRTUAL_ADDRESS GetCustomCBVAddress() const { return customCBVAddress_; }

    // Color controls
    void SetColor(const Vector4& color);
    void SetEnvironmentCoefficient(float coefficient);
    void SetInstanceColor(uint32_t index, const Vector4& color);
    void SetAllInstanceColor(const Vector4& color);

    // Instances
    void AddInstance(const Transform& t);
    void AddInstance(const Transform& t, const Vector4& color);
    void AddInstance(const Vector3& center, float scale = 1.0f, const Vector3& rotate = {0,0,0});
    void AddInstance(const Vector3& center, float scale, const Vector3& rotate, const Vector4& color);
    
    // World matrix based instances
    void AddInstanceWorld(const Matrix4x4& world, const Vector4& color = {1,1,1,1});

    void UpdateInstance(uint32_t index, const Transform& t);
    void ClearInstances();

    virtual void BuildInstanceBuffer(bool force = false);
    void SyncBeforeDraw() override;

    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_[lastUpdateFrameIndex_]; }
    UINT GetInstanceCount() const { return visibleInstanceCount_; }

    // --- GPU Culling Getters ---
    D3D12_GPU_DESCRIPTOR_HANDLE GetOutputInstancesSrvHandleGPU() const { return outputInstanceSrvGPU_[lastUpdateFrameIndex_]; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetOutputInstancesUavHandleGPU() const { return outputInstanceUavGPU_[lastUpdateFrameIndex_]; }
    ID3D12Resource* GetOutputInstanceBuffer() const { return outputInstanceBuffer_[lastUpdateFrameIndex_].Get(); }
    ID3D12Resource* GetIndirectCommandBuffer() const { return indirectCommandBuffer_[lastUpdateFrameIndex_].Get(); }
    ID3D12Resource* GetIndirectCommandUploadBuffer() const { return indirectCommandUploadBuffer_[lastUpdateFrameIndex_].Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetIndirectCommandUavHandleGPU() const { return indirectCommandUavGPU_[lastUpdateFrameIndex_]; }
    D3D12_GPU_VIRTUAL_ADDRESS GetCullingDataAddress() const { return cullingDataBuffer_[lastUpdateFrameIndex_] ? cullingDataBuffer_[lastUpdateFrameIndex_]->GetGPUVirtualAddress() : 0; }
    UINT GetMaxInstanceCount() const { return static_cast<UINT>(instances_.size() + instanceWorlds_.size()); }

protected:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color;
    };

    struct TransformData {
        Vector4 position; // w is padding
        Vector4 rotation; // w is padding
        Vector4 scale;    // w is padding
        Vector4 color;
    };

    struct CullingData {
        Vector4 planes[6];
        uint32_t maxInstanceCount;
        float localRadius;
        float time;
        float padding;
    };

    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void CreateGPUCullingBuffers(uint32_t instanceCount);

    virtual float GetBoundingSphereRadius() const = 0; // for culling

protected:
    static DirectXCommon*  dx_;
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DescriptorPool* srvPool_;

    uint32_t materialCbIndex_ = static_cast<uint32_t>(-1);
    Material cpuMaterialData_{};
    ResourceHandle textureHandle_{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               instancingSrvIndex_{};

    std::vector<Transform> instances_;
    std::vector<Vector4>   instanceColors_;
    std::vector<Matrix4x4> instanceWorlds_;
    std::vector<Vector4>   instanceWorldColors_;

    bool                   instanceDirty_ = false;
    bool                   isCullingEnabled_ = true;
    bool                   useGPUCulling_ = false;
    uint32_t               visibleInstanceCount_ = 0;

    // --- GPU Culling Resources ---
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> outputInstanceBuffer_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            outputInstanceSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            outputInstanceSrvGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               outputInstanceSrvIndex_{};

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            outputInstanceUavCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            outputInstanceUavGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               outputInstanceUavIndex_{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> indirectCommandBuffer_{};
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> indirectCommandUploadBuffer_{};
    
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            indirectCommandUavCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            indirectCommandUavGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               indirectCommandUavIndex_{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> cullingDataBuffer_{};

    uint32_t lastUpdateFrameIndex_ = 0;
    bool isDirty_ = true;

    BlendMode blendMode_ = BlendMode::kBlendModeNormal;
    PSOManager::DepthWrite depthWrite_ = PSOManager::DepthWrite::Enable;
    PSOManager::CullMode cullMode_ = PSOManager::CullMode::Back;
    bool castShadows_ = true;
    ID3D12PipelineState* customPSO_ = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS customCBVAddress_ = 0;
};
