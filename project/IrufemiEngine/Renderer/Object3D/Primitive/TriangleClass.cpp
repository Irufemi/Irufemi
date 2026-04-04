#include "TriangleClass.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include <string>
#include <algorithm>

TextureManager* TriangleClass::textureManager_ = nullptr;
DrawManager* TriangleClass::drawManager_ = nullptr;
DebugUI* TriangleClass::ui_ = nullptr;

void TriangleClass::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;
    resource_ = std::make_unique<Object3DResource>();

    // PrimitiveManager から標準の形状データを取得
    const auto& primitiveData = PrimitiveManager::GetInstance()->GetPrimitiveData(PrimitiveType::Triangle);
    resource_->vertexDataList_ = primitiveData.vertices;
    resource_->indexDataList_ = primitiveData.indices;

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    // 頂点/インデックスデータのコピー
    if (resource_->vertexData_) {
        std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);
    }
    if (resource_->indexData_) {
        std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);
    }

    // 初回の行列計算
    resource_->UpdateTransform(*camera_);

    // マテリアル初期設定
    if (resource_->materialData_) {
        resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
        resource_->materialData_->enableLighting = true;
        resource_->materialData_->hasTexture = true;
        resource_->materialData_->lightingMode = 3;
        resource_->materialData_->uvTransform = Math::MakeIdentity4x4();
        resource_->materialData_->metallic = 0.0f;
        resource_->materialData_->roughness = 0.5f;
        resource_->materialData_->environmentCoefficient = 0.0f;
    }

    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        
        // デバッグUI用のインデックス更新
        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void TriangleClass::Update() {
    if (!resource_ || !camera_) return;

    // 行列更新
    resource_->UpdateTransform(*camera_);

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

    // 共有リソースを使用して描画
    drawManager_->DrawStandard3D(resource_.get());
}

void TriangleClass::Debug(const char* triangleName) {
#if defined USE_IMGUI
    std::string name = std::string("Triangle: ") + triangleName;
    ImGui::Begin(name.c_str());
    if (ui_) {
        ui_->DebugTransform(resource_->transform_);
        ui_->DebugMaterialBy3D(resource_->materialData_);
        ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
        ui_->DebugUvTransform(resource_->uvTransform_);
    }
    ImGui::End();
#endif
    Update();
}