#include "TriangleClass.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"

#include <algorithm>

TextureManager* TriangleClass::textureManager_ = nullptr;
DrawManager* TriangleClass::drawManager_ = nullptr;
DebugUI* TriangleClass::ui_ = nullptr;

void TriangleClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // D3D12ResourceUtilを生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // 入力頂点は1点のみ(GS で増やす)
    resource_->vertexDataList_.clear();
    resource_->vertexDataList_.push_back({ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } ,{0.0f,0.0f,-1.0f} });

    // インデックスは使わない(空でOK)
    resource_->indexDataList_.clear();

    // リソース確保と Map
    resource_->CreateResource();
    resource_->Map();

    // VBV 設定とデータ転送
    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());
    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    // IBV は未使用だが、0要素のままで問題なし(DrawTriangle では IB を設定しない)
    resource_->indexBufferView_ = {};
    if (resource_->indexResource_) {
        resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
        resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(resource_->indexDataList_.size());
        resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
        if (!resource_->indexDataList_.empty()) {
            std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);
        }
    }

    // マテリアル
    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = true;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();
    resource_->materialData_->shininess = 64.0f;

    // WVP
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線用行列
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f; worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    // テクスチャ
    auto textureNames = textureManager_->GetTextureNames();
    std::sort(textureNames.begin(), textureNames.end());
    if (!textureNames.empty()) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void TriangleClass::Update() {

    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f; worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    if (resource_->transformationData_) {
        *resource_->transformationData_ = {
            resource_->transformationMatrix_.WVP,
            resource_->transformationMatrix_.world,
            resource_->transformationMatrix_.WorldInverseTranspose
        };
    }

    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void TriangleClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }

    // POINTLIST で 1 点入力 → GS で生成
    drawManager_->DrawTriangle(this);
}

void TriangleClass::Debug([[maybe_unused]] const char* triangleName) {
#if defined USE_IMGUI
    std::string name = std::string("Triangle: ") + triangleName;
    ImGui::Begin(name.c_str());
    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);
    ImGui::End();
#endif
}