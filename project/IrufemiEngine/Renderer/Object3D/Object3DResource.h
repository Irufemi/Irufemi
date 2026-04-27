#pragma once
#include "../Core/BaseResource.h"
#include <vector>
#include "../../Engine/Graphics/DirectX/DirectXCommon.h"
#include <wrl.h>
#include <d3d12.h>
#include "../VertexData.h"
#include "../../Engine/Graphics/Data/Material.h"
#include "../TransformationMatrix.h"
#include "../../Engine/Core/Math/Transform.h"
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"

class Camera;

class Object3DResource : public BaseResource {
public:
    virtual ~Object3DResource();

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
    
    ConstantBuffer<TransformationMatrix> internalTransformationBuffer_;
    ConstantBuffer<TransformationMatrix>* externalTransformationBuffer_ = nullptr;
    
    const TransformationMatrix& GetTransformationMatrix() const { return transformationMatrix_; }

    // --- getters ---
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const {
        return materialBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformVAddress() const {
        if (externalTransformationBuffer_) {
            return externalTransformationBuffer_->GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
        }
        return internalTransformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }
    
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};
    
    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

    
    void SyncBeforeDraw() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        
        if (isDirtyBuffer_[frameIndex]) {
            // 外部バッファがなければ自身を更新
            if (!externalTransformationBuffer_) {
                internalTransformationBuffer_.Update(transformationMatrix_, frameIndex);
            }
            
            // マテリアルデータを更新
            materialBuffer_.Update(cpuMaterialData_, frameIndex);
            
            isDirtyBuffer_[frameIndex] = false;
        }
    }
    
    // --- 外部リソースの借用 (ObjClass/AnimationModel等で共有するため) ---
    void SetExternalTransformationBuffer(ConstantBuffer<TransformationMatrix>* externalBuffer) {
        externalTransformationBuffer_ = externalBuffer;
    }

    // --- テクスチャ ---
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    bool isFirstUpdate_ = true;
};
