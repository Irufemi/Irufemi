#include "Object2DResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Math.h"

Object2DResource::~Object2DResource() {
    Unmap();
}

void Object2DResource::CreateResource() {
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

    materialBuffer_.Initialize(s_dxCommon_);
    for(uint32_t i=0; i<kMaxFramesInFlight; ++i){
        materialBuffer_[i]->color = {1,1,1,1};
        materialBuffer_[i]->enableLighting = true;
        materialBuffer_[i]->uvTransform = Math::MakeIdentity4x4();
        materialBuffer_[i]->metallic = 0.0f;
        materialBuffer_[i]->roughness = 0.5f;
        materialBuffer_[i]->environmentCoefficient = 0.0f;
    }

    transformationBuffer_.Initialize(s_dxCommon_);
}

void Object2DResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
}

void Object2DResource::Unmap() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
        vertexData_ = nullptr;
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
        indexData_ = nullptr;
    }
}

void Object2DResource::UpdateTransform(const Camera& camera) {
    
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // 2D なので正射影行列を掛ける
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, camera.GetOrthographicMatrix());

    // CPU側のマテリアルキャッシュにのみ反映させる
    cpuMaterialData_.uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}
