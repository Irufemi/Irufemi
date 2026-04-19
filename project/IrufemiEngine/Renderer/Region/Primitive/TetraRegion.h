#pragma once

#include <vector>
#include <memory>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <cassert>
#include <array>
#include "Engine/Graphics/DirectX/DirectXCommon.h"

#include "Renderer/VertexData.h"
#include "Engine/Graphics/DirectX/ConstantBuffer.h"
#include "Engine/Graphics/Data/Material.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Transform.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Vector4.h"
#include "Engine/Graphics/Data/DirectionalLight.h"
#include "Engine/Graphics/Data/CameraForGPU.h"

class DirectXCommon;
class Camera;                       
class TextureManager;
class DrawManager;
class DescriptorPool; // 追加

class TetraRegion {
public:
    TetraRegion() {
        instancingSrvIndex_.fill(UINT32_MAX);
    }
    static void SetDirectXCommon(DirectXCommon* dx);
    static void SetTextureManager(TextureManager* tm);
    static void SetDrawManager(DrawManager* dm);
    static void SetSrvAllocator(DescriptorPool* alloc) { srvPool_ = alloc; } // 追加
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

    ~TetraRegion(); // SRV遅延解放

    void Initialize(Camera* camera, const std::string& textureName = "resources/uvChecker.png");

    // Transformベース
    void AddInstance(const Transform& t);
    void AddInstance(const Transform& t, const Vector4& color); // 追加: 色指定
    void AddInstance(const Vector3& center, float scale = 1.0f, const Vector3& rotate = {0,0,0});
    void AddInstance(const Vector3& center, float scale, const Vector3& rotate, const Vector4& color); // 追加: 色指定

    // World行列ベース
    void AddInstanceWorld(const Matrix4x4& world, const Vector4& color = {1,1,1,1});

    void ClearInstances();
    void BuildInstanceBuffer(bool force = false);
    void Draw();

    // 色設定API(SphereRegion と同等)
    void SetColor(const Vector4& color);                 // マテリアル色
    void SetInstanceColor(uint32_t index, const Vector4& color); // 個別インスタンス色(Transform系)
    void SetAllInstanceColor(const Vector4& color);      // 全インスタンス同色(Transform系)

    // サイズ関連
    void SetEdge(float edge);
    float GetModelVertexRadius() const;
    float ComputeScaleFromVertexRadius(float worldVertexRadius) const;
    void AddInstanceByVertexRadius(const Vector3& center, float worldVertexRadius, const Vector3& rotate = {0,0,0});

    ID3D12Resource*                        GetMaterialResource() { return materialBuffer_.GetResource(dx_->GetFrameIndex()); }

    UINT                        GetInstanceCount() const {
        return visibleInstanceCount_;
    }

private:
    struct InstanceData {
        Matrix4x4 WVP;
        Matrix4x4 World;
        Matrix4x4 WorldInverseTranspose;
        Vector4   color;
    };

    void BuildTetraMesh(std::vector<VertexData>& outVertices, std::vector<uint32_t>& outIndices);
    void CreateMeshBuffers(const std::vector<VertexData>& vertices, const std::vector<uint32_t>& indices);
    void CreateMaterialResources();
    void EnsureSharedTexture(const std::string& textureName);
    void EnsureLightAndCamera();
    void CreateOrResizeInstanceBuffer(uint32_t instanceCount);

private:
    static DirectXCommon* dx_;
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DescriptorPool* srvPool_; // 追加

    Camera* camera_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW               vertexBufferView_{};
    UINT                                   vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW                indexBufferView_{};
    UINT                                   indexCount_ = 0;

    // マテリアル
    ConstantBuffer<Material> materialBuffer_;

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kMaxFramesInFlight> instanceBuffer_{};
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvCPU_{};
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, kMaxFramesInFlight>            instancingSrvGPU_{};
    std::array<uint32_t, kMaxFramesInFlight>                               instancingSrvIndex_{};

    // Transformベース
    std::vector<Transform> instances_;
    std::vector<Vector4>   instanceColors_; // 追加: Transform系の色
    bool                   instanceDirty_ = false;

    // World行列ベース
    std::vector<Matrix4x4> instanceWorlds_;
    std::vector<Vector4>   instanceWorldColors_;

    // モデル辺長
    float edgeLength_ = 1.0f;
    bool  meshDirty_ = false;

    bool     isCullingEnabled_ = true;
    uint32_t visibleInstanceCount_ = 0;
};