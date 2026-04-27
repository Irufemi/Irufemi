#pragma once
#include "../Core/BaseResource.h"
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"
#include "../VertexData.h"
#include "../../Engine/Graphics/Data/Material.h"
#include "../TransformationMatrix.h"
#include "../../Engine/Core/Math/Transform.h"

class Camera;

class LineResource : public BaseResource {
public:
    virtual ~LineResource();

    void CreateResource() override;
    void Map() override;
    void Unmap() override;

    void UpdateTransform(const Camera& camera);

public:
    // --- 頂点バッファ ---
    VertexData* vertexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // --- インデックスバッファ ---
    uint32_t* indexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;

    // --- マテリアル ---
    Material cpuMaterialData_{};
    ConstantBuffer<Material> materialBuffer_;

    // --- トランスフォーム ---
    Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    TransformationMatrix transformationMatrix_{};
    ConstantBuffer<TransformationMatrix> transformationBuffer_;

    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const {
        return materialBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const {
        return transformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }

    void SyncBeforeDraw() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        transformationBuffer_.Update(transformationMatrix_, frameIndex);
        materialBuffer_.Update(cpuMaterialData_, frameIndex);
    }
};
