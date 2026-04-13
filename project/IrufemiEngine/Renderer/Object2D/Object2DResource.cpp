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

    if (!materialResource_) {
        materialResource_ = s_dxCommon_->CreateBufferResource(sizeof(Material));
    }
    if (!transformationResource_) {
        transformationResource_ = s_dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    }
}

void Object2DResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    if (materialResource_) {
        materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    }
    if (transformationResource_) {
        transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
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
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
        materialData_ = nullptr;
    }
    if (transformationResource_) {
        transformationResource_->Unmap(0, nullptr);
        transformationData_ = nullptr;
    }
}

void Object2DResource::UpdateTransform(const Camera& camera) {
    if (!transformationData_) return;

    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // 2D なので正射影行列を掛ける
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, camera.GetOrthographicMatrix());

    *transformationData_ = transformationMatrix_;

    // UVTransform も更新
    if (materialData_) {
        materialData_->uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
    }
}
