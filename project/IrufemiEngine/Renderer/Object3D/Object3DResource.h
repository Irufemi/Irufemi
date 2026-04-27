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
    // --- 鬆らせ繝舌ャ繝輔ぃ ---
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // --- 繧､繝ｳ繝・ャ繧ｯ繧ｹ繝舌ャ繝輔ぃ ---
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;

    // --- 繝槭ユ繝ｪ繧｢繝ｫ ---
    Transform uvTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Material cpuMaterialData_{};
    Material* GetMaterialData() { return &cpuMaterialData_; }
    
    ConstantBuffer<Material> materialBuffer_;

    // --- 繝医Λ繝ｳ繧ｹ繝輔か繝ｼ繝 ---
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
    
    void SyncBeforeDraw() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        if (!isDirtyBuffer_[frameIndex]) return;
        isDirtyBuffer_[frameIndex] = false;
        
        // 螟夜Κ繝舌ャ繝輔ぃ縺後↑縺代ｌ縺ｰ閾ｪ霄ｫ繧呈峩譁ｰ
        if (!externalTransformationBuffer_) {
            internalTransformationBuffer_.Update(transformationMatrix_, frameIndex);
        }
        
        // 繝槭ユ繝ｪ繧｢繝ｫ繝・・繧ｿ繧呈峩譁ｰ
        materialBuffer_.Update(cpuMaterialData_, frameIndex);
    }
    
    // --- 螟夜Κ繝ｪ繧ｽ繝ｼ繧ｹ縺ｮ蛟溽畑 (ObjClass/AnimationModel遲峨〒蜈ｱ譛峨☆繧九◆繧・ ---
    void SetExternalTransformationBuffer(ConstantBuffer<TransformationMatrix>* externalBuffer) {
        externalTransformationBuffer_ = externalBuffer;
    }

    // --- 繝・け繧ｹ繝√Ε ---
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

private:
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};
};
