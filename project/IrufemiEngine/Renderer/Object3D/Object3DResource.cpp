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

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (!materialResource_[i]) {
            materialResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(Material));
        }
        // 外部から借用していない場合のみ内部で作成
        if (!transformationResource_[i]) {
            transformationResource_[i] = s_dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
        }
    }
}

void Object3DResource::Map() {
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

void Object3DResource::Unmap() {
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
        // 外部バッファの借用中はこのクラス自体では Unmap しない
        if (transformationResource_[i]) {
            transformationResource_[i]->Unmap(0, nullptr);
            transformationData_[i] = nullptr;
        }
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

    uint32_t frameIndex = BaseResource::GetDirectXCommon()->GetFrameIndex();
    if (transformationData_[frameIndex]) {
        *transformationData_[frameIndex] = transformationMatrix_;
    }

    // マテリアルの CPUキャッシュ更新 (明示的な SyncMaterialData() 呼び出しが必要)
    cpuMaterialData_.uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
}

void Object3DResource::SetExternalTransformationResource(Microsoft::WRL::ComPtr<ID3D12Resource>* resources, TransformationMatrix** data) {
    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        transformationResource_[i] = resources[i];
        transformationData_[i] = data[i];
    }
}
