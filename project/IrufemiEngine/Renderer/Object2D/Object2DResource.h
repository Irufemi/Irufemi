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
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"

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
    ConstantBuffer<Material> materialBuffer_;

    // --- トランスフォーム ---
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    TransformationMatrix transformationMatrix_{};
    ConstantBuffer<TransformationMatrix> transformationBuffer_;

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const {
        return transformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const {
        return materialBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }
    
    void SyncMaterialData() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        materialBuffer_.Update(cpuMaterialData_, frameIndex);
    }

    int32_t dirtyFramesLeft_ = kMaxFramesInFlight; // 変更があったら指定フレーム数だけ全バッファに伝播させる
    uint32_t lastSyncedFrameIndex_ = UINT32_MAX;
    void MakeDirty() { dirtyFramesLeft_ = kMaxFramesInFlight; }

    void SyncIfDirty() {
        if (dirtyFramesLeft_ > 0) {
            uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
            transformationBuffer_.Update(transformationMatrix_, frameIndex);
            SyncMaterialData();
            
            if (lastSyncedFrameIndex_ != frameIndex) {
                dirtyFramesLeft_--;
                lastSyncedFrameIndex_ = frameIndex;
            }
        }
    }
};
