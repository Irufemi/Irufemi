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

    materialBuffer_.Initialize(s_dxCommon_);
    for(uint32_t i=0; i<kMaxFramesInFlight; ++i){
        materialBuffer_[i]->color = {1,1,1,1};
        materialBuffer_[i]->enableLighting = true;
        materialBuffer_[i]->uvTransform = Math::MakeIdentity4x4();
        materialBuffer_[i]->metallic = 0.0f;
        materialBuffer_[i]->roughness = 0.5f;
        materialBuffer_[i]->environmentCoefficient = 0.0f;
    }

    internalTransformationBuffer_.Initialize(s_dxCommon_);
}

void Object3DResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
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
}

void Object3DResource::UpdateTransform(const Camera& camera) {


    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    // CPU側のマテリアルキャッシュにのみ反映させる

    // 法線変換用：平行移動を除いた World を使う
    Matrix4x4 worldForNormal = transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    
    // 逆転置行列を計算
    transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    // マテリアルの CPUキャッシュ更新 (描画直前の SyncBeforeDraw で GPUへ送られる)
    cpuMaterialData_.uvTransform = Math::MakeAffineMatrix(uvTransform_.scale, uvTransform_.rotate, uvTransform_.translate);
    MarkAsDirty();
}
