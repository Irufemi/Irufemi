#pragma once
#include "../Core/BaseResource.h"
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include "../VertexData.h"
#include "../../Engine/Graphics/Data/Material.h"
#include "Data/Particle.h"
#include "../../Engine/Core/Math/Transform.h"
#include "../../Engine/Graphics/DirectX/DirectXCommon.h"
#include "../../Engine/Graphics/DirectX/ConstantBuffer.h"

class ParticleResource : public BaseResource {
public:
    virtual ~ParticleResource();

    void CreateResource() override;
    void Map() override;
    void Unmap() override;

public:
    // --- 鬯・ｉ縺帷ｹ晁・繝｣郢晁ｼ斐＜ ---
    std::vector<VertexData> vertexDataList_{};
    VertexData* vertexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // --- 郢ｧ・､郢晢ｽｳ郢昴・繝｣郢ｧ・ｯ郢ｧ・ｹ郢晁・繝｣郢晁ｼ斐＜ ---
    std::vector<uint32_t> indexDataList_{};
    uint32_t* indexData_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
    uint32_t indexCount_ = 0;

    // --- 郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ ---
    Transform uvTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Material cpuMaterialData_{};
    Material* GetMaterialData() { return &cpuMaterialData_; }
    
    ConstantBuffer<Material> materialBuffer_;

    // --- 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｷ郢晢ｽｳ郢ｧ・ｰ郢晁・繝｣郢晁ｼ斐＜ (StructuredBuffer) ---
    static constexpr uint32_t kNumMaxInstance = 4096;
    ParticleForGPU* instancingData_[kMaxFramesInFlight] = { nullptr };
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_[kMaxFramesInFlight];
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_[kMaxFramesInFlight]{};

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    D3D12_GPU_VIRTUAL_ADDRESS GetInstancingVAddress() const {
        return instancingResource_[BaseResource::GetDirectXCommon()->GetFrameIndex()]->GetGPUVirtualAddress();
    }
    
    void SyncBeforeDraw() {
        uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
        if (!isDirtyBuffer_[frameIndex]) return;
        isDirtyBuffer_[frameIndex] = false;
        materialBuffer_.Update(cpuMaterialData_, frameIndex);
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const {
        return materialBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }

    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

private:
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};
};
