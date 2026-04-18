#pragma once
#include "../Core/BaseResource.h"
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "../VertexData.h"
#include "../../Engine/Graphics/Data/Material.h"
#include "../TransformationMatrix.h"
#include "../../Engine/Core/Math/Transform.h"
#include "../../Engine/Graphics/DirectX/DirectXCommon.h"

class Camera;

class Object2DResource : public BaseResource {
public:
    virtual ~Object2DResource();

    void CreateResource() override;
    void Map() override;
    void Unmap() override;

    void UpdateTransform(const Camera& camera);

public:
    // --- 頂点バッファ ---
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // --- インデックスバッファ ---
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;

    // --- マテリアル ---
    Transform uvTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Material cpuMaterialData_{};
    Material* GetMaterialData() { return &cpuMaterialData_; }
    Material* materialData_[kMaxFramesInFlight] = { nullptr };
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_[kMaxFramesInFlight];

    // --- トランスフォーム ---
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    TransformationMatrix transformationMatrix_{};
    TransformationMatrix* transformationData_[kMaxFramesInFlight] = { nullptr };
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_[kMaxFramesInFlight];

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const {
        return transformationResource_[BaseResource::GetDirectXCommon()->GetFrameIndex()]->GetGPUVirtualAddress();
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const {
        return materialResource_[BaseResource::GetDirectXCommon()->GetFrameIndex()]->GetGPUVirtualAddress();
    }
    
    void SyncMaterialData() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        if (materialData_[frameIndex]) {
            *materialData_[frameIndex] = cpuMaterialData_;
        }
    }

    void SyncGPUData() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        if (transformationData_[frameIndex]) {
            *transformationData_[frameIndex] = transformationMatrix_;
        }
        SyncMaterialData();
    }
};
