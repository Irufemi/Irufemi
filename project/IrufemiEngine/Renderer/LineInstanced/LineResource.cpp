#include "LineResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Math.h"

LineResource::~LineResource() {
    Unmap();
}

void LineResource::CreateResource() {
    if (!s_dxCommon_) return;

    // Line は基本 2 頂点
    if (!vertexResource_) {
        vertexResource_ = s_dxCommon_->CreateBufferResource(sizeof(VertexData) * 2);
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(VertexData) * 2;
        vertexBufferView_.StrideInBytes = sizeof(VertexData);
    }

    if (!indexResource_) {
        indexResource_ = s_dxCommon_->CreateBufferResource(sizeof(uint32_t) * 2);
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 2;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        indexCount_ = 2;
    }

    materialBuffer_.Initialize(s_dxCommon_);
    for(uint32_t i=0; i<kMaxFramesInFlight; ++i){
        materialBuffer_[i]->color = {1,1,1,1};
        materialBuffer_[i]->uvTransform = Math::MakeIdentity4x4();
    }
    transformationBuffer_.Initialize(s_dxCommon_);
}

void LineResource::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
 
}

void LineResource::Unmap() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
        vertexData_ = nullptr;
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
        indexData_ = nullptr;
    }
 
}

void LineResource::UpdateTransform(const Camera& camera) {
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera.GetViewMatrix(), camera.GetPerspectiveFovMatrix()));

    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera.GetViewMatrix(), camera.GetPerspectiveFovMatrix()));
    MarkAsDirty();
}
