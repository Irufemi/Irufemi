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

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (!materialResource_[i]) {
            materialResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(Material));
        }
        if (!transformationResource_[i]) {
            transformationResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
        }
    }
}

void Object2DResource::Map() {
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
        if (transformationResource_[i]) {
            transformationResource_[i]->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_[i]));
        }
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
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (materialResource_[i]) {
            materialResource_[i]->Unmap(0, nullptr);
            materialData_[i] = nullptr;
        }
        if (transformationResource_[i]) {
            transformationResource_[i]->Unmap(0, nullptr);
            transformationData_[i] = nullptr;
        }
    }
}

void Object2DResource::UpdateTransform(const Camera& camera) {
    if (!transformationData_) return;

    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // 2D なので正射影行列を掛ける
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, camera.GetOrthographicMatrix());

    uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
    if (transformationData_[frameIndex]) {
        *transformationData_[frameIndex] = transformationMatrix_;
    }

    // CPU側のマテリアルキャッシュにのみ反映させる
    cpuMaterialData_.uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}
