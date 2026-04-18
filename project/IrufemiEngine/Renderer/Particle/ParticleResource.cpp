#include "ParticleResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/Math/Math.h"

ParticleResource::~ParticleResource() {
    Unmap();
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

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (!materialResource_[i]) {
            materialResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(Material));
        }
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
        if (materialResource_[i]) {
            materialResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&materialData_[i]));
        }
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
        if (materialResource_[i]) {
            materialResource_[i]->Unmap(0, nullptr);
            materialData_[i] = nullptr;
        }
        if (instancingResource_[i]) {
            instancingResource_[i]->Unmap(0, nullptr);
            instancingData_[i] = nullptr;
        }
    }
}
