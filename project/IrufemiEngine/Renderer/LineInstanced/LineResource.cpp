#include "LineResource.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Geometry/Math.h"

LineResource::~LineResource() {
    Unmap();
}

void LineResource::CreateResource() {
    if (!s_dxCommon_) return;

    // Line は基本 2 頂点
    if (!vertexResource_) {
        vertexResource_ = s_dxCommon_->CreateBufferResource(sizeof(LineVertexData) * 2);
        vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
        vertexBufferView_.SizeInBytes = sizeof(LineVertexData) * 2;
        vertexBufferView_.StrideInBytes = sizeof(LineVertexData);
    }

    if (!indexResource_) {
        indexResource_ = s_dxCommon_->CreateBufferResource(sizeof(uint32_t) * 2);
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes = sizeof(uint32_t) * 2;
        indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        indexCount_ = 2;
    }

    if (!materialResource_) {
        materialResource_ = s_dxCommon_->CreateBufferResource(sizeof(LineMaterial));
    }
    if (!transformationResource_) {
        transformationResource_ = s_dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    }
}

void LineResource::Map() {
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

void LineResource::Unmap() {
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

void LineResource::UpdateTransform(const Camera& camera) {
    if (!transformationData_) return;

    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera.GetViewMatrix(), camera.GetPerspectiveFovMatrix()));

    *transformationData_ = transformationMatrix_;
}
