#include "D3D12ResourceUtil.h"

#include "engine/directX/DirectXCommon.h"
#include "camera/Camera.h"

DirectXCommon* D3D12ResourceUtil::dxCommon_ = nullptr;
DirectXCommon* D3D12ResourceUtilParticle::dxCommon_ = nullptr;

//デストラクタ
D3D12ResourceUtil::~D3D12ResourceUtil() {
    char buf[256];
    snprintf(buf, sizeof(buf), "[D3D12ResourceUtil] Destruct: vertex=%p, index=%p, material=%p, trans=%p, light=%p\n",
        vertexResource_.Get(), indexResource_.Get(), materialResource_.Get(), transformationResource_.Get(), directionalLightResource_.Get());
    OutputDebugStringA(buf);

    UnMap();
    if (vertexResource_) { vertexResource_.Reset(); }
    if (indexResource_) { indexResource_.Reset(); }
    if (materialResource_) { materialResource_.Reset(); }
    if (transformationResource_) { transformationResource_.Reset(); }
    if (directionalLightResource_) { directionalLightResource_.Reset(); }
    if (cameraResource_) { cameraResource_.Reset(); }
}


//ID3D12Resourceを生成する
void D3D12ResourceUtil::CreateResource() {
    char buf[256];
    if (!vertexDataList_.empty()) {
        vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * static_cast<size_t>(vertexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", vertexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    if (!indexDataList_.empty()) {
        indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * static_cast<size_t>(indexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", indexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));
    snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", materialResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
    transformationResource_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", transformationResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
    directionalLightResource_ = dxCommon_->CreateBufferResource(sizeof(DirectionalLight));
    snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", directionalLightResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
    cameraResource_ = dxCommon_->CreateBufferResource(sizeof(CameraForGPU));
    snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", cameraResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
}

//バッファへの書き込みを開放
void D3D12ResourceUtil::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
    directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
}

//バッファへの書き込みを閉鎖
void D3D12ResourceUtil::UnMap() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
    }
    if (materialResource_) {
        materialResource_->Unmap(0, nullptr);
    }
    if (transformationResource_) {
        transformationResource_->Unmap(0, nullptr);
    }
    if (directionalLightResource_) {
        directionalLightResource_->Unmap(0, nullptr);
    }
}

void D3D12ResourceUtil::UpdateTransform3D(const Camera& camera) {
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    transformationMatrix_.WVP = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera.GetViewMatrix(), camera.GetPerspectiveFovMatrix()));// 法線変換用：平行移動を除いた World を使う
    Matrix4x4 worldForNormal = transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    // 逆転置行列を計算
    transformationMatrix_.WorldInverseTranspose =
        Math::Transpose(Math::Inverse(worldForNormal));
}



//デストラクタ
D3D12ResourceUtilParticle::~D3D12ResourceUtilParticle() {
    char buf[256];
    snprintf(buf, sizeof(buf), "[D3D12ResourceUtil] Destruct: vertex=%p, index=%p, material=%p\n",
        vertexResource_.Get(), indexResource_.Get(), materialResource_.Get());
    OutputDebugStringA(buf);

    UnMap();
    if (vertexResource_) { vertexResource_.Reset(); }
    if (indexResource_) { indexResource_.Reset(); }
    if (materialResource_) { materialResource_.Reset(); }
}


//ID3D12Resourceを生成する
void D3D12ResourceUtilParticle::CreateResource() {
    char buf[256];
    if (!vertexDataList_.empty()) {
        vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * static_cast<size_t>(vertexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", vertexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    if (!indexDataList_.empty()) {
        indexResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * static_cast<size_t>(indexDataList_.size()));
        snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", indexResource_.Get(), __FILE__, __LINE__);
        OutputDebugStringA(buf);
    }
    materialResource_ = dxCommon_->CreateBufferResource(sizeof(Material));
    snprintf(buf, sizeof(buf), "Created ID3D12Resource at %p in %s:%d\n", materialResource_.Get(), __FILE__, __LINE__);
    OutputDebugStringA(buf);
}

//バッファへの書き込みを開放
void D3D12ResourceUtilParticle::Map() {
    if (vertexResource_) {
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    }
    if (indexResource_) {
        indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    }
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
}

//バッファへの書き込みを閉鎖
void D3D12ResourceUtilParticle::UnMap() {
    if (vertexResource_) {
        vertexResource_->Unmap(0, nullptr);
    }
    if (indexResource_) {
        indexResource_->Unmap(0, nullptr);
    }
    materialResource_->Unmap(0, nullptr);
}