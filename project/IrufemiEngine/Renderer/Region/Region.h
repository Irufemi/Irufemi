#include "../Core/IRenderable.h"
#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Graphics/DirectX/ConstantBuffer.h"
#include "Engine/Graphics/Data/Material.h"

#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Transform.h"
#include "Renderer/VertexData.h"
#include "Resource/Model/Data/ObjModel.h"

// 前方宣言
class Camera;
class DirectXCommon;
class TextureManager;
class DrawManager;
class ModelManager;
class DescriptorPool;
struct ManagedModel;
struct GpuMesh;

class ModelRegion : public IRenderable {
public:
    ModelRegion() {
        instancingSrvIndex_.fill(UINT32_MAX);
    }
    ~ModelRegion();

    void Initialize(Camera* camera, const std::string& objFilename);
    void AddInstance(const Transform& t);
    void ClearInstances();
    void BuildInstanceBuffer(bool force = false);
    void SyncBeforeDraw() override;
    void Draw() override;

    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
    static void SetSrvAllocator(DescriptorPool* alloc) { srvPool_ = alloc; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

    // --- DrawManager から参照する Getter 群 ---
    const GpuMesh* GetGpuMesh() const; // 共有メッシュ取得
    ID3D12Resource* GetMaterialResource() { return materialBuffer_.GetResource(dx_->GetFrameIndex()); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_[lastUpdateFrameIndex_]; }
    UINT GetInstanceCount() const { return visibleInstanceCount_; }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color;
    };

    // リソース生成ヘルパ
    void CreateMaterialResources(const ObjMesh& mesh);
    void EnsureSharedTexture(const ObjMesh& mesh);
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);
    void InitializeResources();

private:
    static DirectXCommon*  dx_;
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static ModelManager*    modelManager_;
    static DescriptorPool* srvPool_;

    Camera* camera_ = nullptr;

    // 共有モデルデータ(CPU/GPU)
    std::shared_ptr<ManagedModel> managedModel_{};

    // インスタンス固有リソース
    ConstantBuffer<Material> materialBuffer_;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    // インスタンシング用
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               instancingSrvIndex_{};

    std::vector<Transform> instances_;
    bool                   instanceDirty_ = false;
    bool                   isCullingEnabled_ = true;
    bool                   isResourcesInitialized_ = false;
    uint32_t               visibleInstanceCount_ = 0;

    uint32_t lastUpdateFrameIndex_ = 0;
    bool isDirty_ = true;

    // 行列更新の最適化用
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
};
