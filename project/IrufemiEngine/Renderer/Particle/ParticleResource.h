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
#include "../../Engine/Graphics/DirectX/DynamicConstantBuffer.h"

class ParticleResource : public BaseResource {
public:
    virtual ~ParticleResource();

    void CreateResource() override;
    void Map() override;
    void Unmap() override;

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
    
    uint32_t materialCbIndex_ = static_cast<uint32_t>(-1);

    // --- インスタンシングバッファ (StructuredBuffer) ---
    static constexpr uint32_t kNumMaxInstance = 4096;
    ParticleForGPU* instancingData_[kMaxFramesInFlight] = { nullptr };
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_[kMaxFramesInFlight];
    D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU_[kMaxFramesInFlight]{};

    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_ = {};

    D3D12_GPU_VIRTUAL_ADDRESS GetInstancingVAddress() const {
        return instancingResource_[BaseResource::GetDirectXCommon()->GetFrameIndex()]->GetGPUVirtualAddress();
    }
    
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};
    
    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }
    
    void SyncBeforeDraw();
    D3D12_GPU_VIRTUAL_ADDRESS GetMaterialVAddress() const;
};
