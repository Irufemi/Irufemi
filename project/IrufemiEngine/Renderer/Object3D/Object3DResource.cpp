#include "Object3DResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Math.h"

Object3DResource::~Object3DResource() {
    Unmap();
}

void Object3DResource::CreateResource() {
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

void Object3DResource::Map() {
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

void Object3DResource::Unmap() {
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

void Object3DResource::UpdateTransform(const Camera& camera) {
    if (!transformationData_) return;

    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera.GetViewMatrix(), camera.GetPerspectiveFovMatrix()));

    // 法線変換用：平行移動を除いた World を使う
    Matrix4x4 worldForNormal = transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    
    // 逆転置行列を計算
    transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    *transformationData_ = transformationMatrix_;

    // マテリアルの UVTransform 更新
    if (materialData_) {
        materialData_->uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
    }
}

void Object3DResource::SetExternalTransformationResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, TransformationMatrix* data) {
    transformationResource_ = resource;
    transformationData_ = data;
}
