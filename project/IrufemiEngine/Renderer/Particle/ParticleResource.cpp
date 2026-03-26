#include "ParticleResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Core/Math/Geometry/Math.h"

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

    if (!materialResource_) {
        materialResource_ = s_dxCommon_->CreateBufferResource(sizeof(ParticleMaterial));
    }

    // インスタンシング用バッファの作成
    if (!instancingResource_) {
        instancingResource_ = s_dxCommon_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
    }
}

void ParticleResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    if (materialResource_) {
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    }
    if (instancingResource_) {
        instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingData_));
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
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
        materialData_ = nullptr;
    }
    if (instancingResource_) {
        instancingResource_->Unmap(0, nullptr);
        instancingData_ = nullptr;
    }
}
