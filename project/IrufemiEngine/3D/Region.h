#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>

#include "function/Math.h"
#include "math/Transform.h"
#include "math/VertexData.h"
#include "math/ObjModel.h"

// 前方宣言
class Camera;
class DirectXCommon;
class TextureManager;
class DrawManager;
class ModelManager;
class DescriptorPool;
struct ManagedModel;
struct GpuMesh;

class Region {
public:
    void Initialize(Camera* camera, const std::string& objFilename);
    void AddInstance(const Transform& t);
    void AddInstance(const Transform& t, const Vector4& color);
    void ClearInstances();
    void BuildInstanceBuffer(bool force = false);
    void Draw();

    static void SetDirectXCommon(DirectXCommon* dx) { dx_ = dx; }
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
    static void SetSrvAllocator(DescriptorPool* alloc) { srvPool_ = alloc; }

    // --- DrawManager から参照する Getter 群 ---
    const GpuMesh* GetGpuMesh() const; // 共有メッシュ取得
    ID3D12Resource* GetMaterialResource() { return materialResource_.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle() const { return textureHandle_; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetInstancingSrvHandleGPU() const { return instancingSrvGPU_; }
    UINT GetInstanceCount() const { return static_cast<UINT>(instances_.size()); }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color;
    };

    struct Instance {
        Transform transform;
        Vector4   color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    // リソース生成ヘルパ
    void CreateMaterialResources(const ObjMesh& mesh);
    void EnsureSharedTexture(const ObjMesh& mesh);
    void EnsureLightAndCamera();
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);

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
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    // インスタンシング用
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE            instancingSrvCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE            instancingSrvGPU_{};
    uint32_t                               instancingSrvIndex_ = UINT32_MAX;

    std::vector<Instance> instances_;
    bool                   instanceDirty_ = false;
};