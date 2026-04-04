#include "Renderer/Object3D/Primitive/CylinderClass.h"

#include <cmath>
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"

#include "Engine/Core/Math/Geometry/Math.h"
#include "Engine/Manager/PrimitiveManager.h"
#include <string>
#include <algorithm>

TextureManager* CylinderClass::textureManager_ = nullptr;
DrawManager* CylinderClass::drawManager_ = nullptr;
DebugUI* CylinderClass::ui_ = nullptr;

void CylinderClass::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;
    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Cylinder);

    // Object3DResource の生成
    resource_ = std::make_unique<Object3DResource>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // ========= リソース確保と書き込み =========
    resource_->CreateResource();
    resource_->Map();

    // マテリアル
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

    // Transform 初期値
    resource_->transform_.translate = info_.center;
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };

    Update();

    // テクスチャ
    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void CylinderClass::Update() {
    if (!resource_ || !camera_) return;

    // Release でも必ず論理情報を実トランスフォームに反映する
    resource_->transform_.translate = info_.center;

    // 実スケール = { radius*scale.x, height*scale.y, radius*scale.z }
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x * 2.0f,
        info_.height * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z * 2.0f
    };

    // 一時的にスケールを上書きして行列更新
    Vector3 originalScale = resource_->transform_.scale;
    resource_->transform_.scale = effectiveScale;
    resource_->UpdateTransform(*camera_);
    resource_->transform_.scale = originalScale;

    if (resource_->materialData_) {
        resource_->materialData_->uvTransform =
            Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
        
        if (resource_->textureHandle_.ptr == 0) {
            resource_->materialData_->hasTexture = false;
        }
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void CylinderClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }

    drawManager_->DrawStandard3D(resource_.get());
}

void CylinderClass::Debug([[maybe_unused]] const char* cylinderName) {

#if defined USE_IMGUI
    std::string name = std::string("Cylinder: ") + cylinderName;
    ImGui::Begin(name.c_str());

    // 中心・半径・高さの編集
    ImGui::DragFloat3("Center", &info_.center.x, 0.01f);
    ImGui::DragFloat("Radius", &info_.radius, 0.01f, 0.001f, 1000.0f);
    ImGui::DragFloat("Height", &info_.height, 0.01f, 0.001f, 1000.0f);

    // Transform(係数スケール・回転・位置)
    resource_->transform_.translate = info_.center;
    ui_->DebugTransform(resource_->transform_);

    // 位置を CylinderInfo に反映
    info_.center = resource_->transform_.translate;

    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();
#endif
}