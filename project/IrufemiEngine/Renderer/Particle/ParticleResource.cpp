#include "ParticleResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/IrufemiEngine.h"

ParticleResource::~ParticleResource() {
    Unmap();
    if (auto engine = BaseResource::GetDirectXCommon()->GetEngine()) {
        if (materialCbIndex_ != static_cast<uint32_t>(-1)) {
            engine->GetMaterialBufferManager()->Free(materialCbIndex_);
        }
    }
}

void ParticleResource::CreateResource() {
    if (!s_dxCommon_) return;

    if (!vertexDataList_.empty()) {
        vertexResource_ = s_dxCommon_->CreateBufferResource(sizeof(VertexData) * vertexDataList_.size());
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexDataList_.size());
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
    }

    if (!indexDataList_.empty()) {
        indexResource_ = s_dxCommon_->CreateBufferResource(sizeof(uint32_t) * indexDataList_.size());
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexDataList_.size());
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        indexCount_ = static_cast<uint32_t>(indexDataList_.size());
    }

    if (auto engine = BaseResource::GetDirectXCommon()->GetEngine()) {
        materialCbIndex_ = engine->GetMaterialBufferManager()->Allocate();
        
        cpuMaterialData_.color = {1,1,1,1};
        cpuMaterialData_.enableLighting = true;
        cpuMaterialData_.uvTransform = Math::MakeIdentity4x4();
        cpuMaterialData_.metallic = 0.0f;
        cpuMaterialData_.roughness = 0.5f;
        cpuMaterialData_.environmentCoefficient = 0.0f;

        for(uint32_t i=0; i<kMaxFramesInFlight; ++i){
            engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, i);
        }
    }

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (!instancingResource_[i]) {
            instancingResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
        }
    }
}

void ParticleResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (instancingResource_[i]) {
            instancingResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_[i]));
        }
    }
}

void ParticleResource::Unmap() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
        vertexData_ = nullptr;
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
        indexData_ = nullptr;
    }
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (instancingResource_[i]) {
            instancingResource_[i]->Unmap(0, nullptr);
            instancingData_[i] = nullptr;
        }
    }
}

D3D12_GPU_VIRTUAL_ADDRESS ParticleResource::GetMaterialVAddress() const {
    if (materialCbIndex_ == static_cast<uint32_t>(-1)) return 0;
    return BaseResource::GetDirectXCommon()->GetEngine()->GetMaterialBufferManager()->GetGPUVirtualAddress(materialCbIndex_, BaseResource::GetDirectXCommon()->GetFrameIndex());
}

void ParticleResource::SyncBeforeDraw() {
    uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
    if (isDirtyBuffer_[frameIndex]) {
        if (auto engine = BaseResource::GetDirectXCommon()->GetEngine()) {
            if (materialCbIndex_ != static_cast<uint32_t>(-1)) {
                engine->GetMaterialBufferManager()->Update(materialCbIndex_, cpuMaterialData_, frameIndex);
            }
        }
        isDirtyBuffer_[frameIndex] = false;
    }
}
