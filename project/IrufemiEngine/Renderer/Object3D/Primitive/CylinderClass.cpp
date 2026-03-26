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

    // D3D12ResourceUtil の生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // ========= リソース確保と書き込み =========
    resource_->CreateResource();
    resource_->Map();

    // マテリアル
    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = true;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();
    resource_->materialData_->shininess = 64.0f;

    // Transform 初期値
    resource_->transform_.translate = info_.center;
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };

    // 実スケール = { radius*scale.x, height*scale.y, radius*scale.z }
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x * 2.0f,
        info_.height * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z * 2.0f
    };

    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);

    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world,
            Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線変換用：平行移動を除く
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    resource_->transformationMatrix_.WorldInverseTranspose =
        Math::Transpose(Math::Inverse(worldForNormal));

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
        selectedTextureIndex_ = (it != textureNames.end())
            ? static_cast<int>(std::distance(textureNames.begin(), it))
            : 0;
    }
}

void CylinderClass::Update() {

    // Release でも必ず論理情報を実トランスフォームに反映する
    resource_->transform_.translate = info_.center;

    // 実スケール = { radius*scale.x, height*scale.y, radius*scale.z }
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x * 2.0f,
        info_.height * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z * 2.0f
    };

    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);

    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world,
            Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線用(平行移動除去)
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    resource_->transformationMatrix_.WorldInverseTranspose =
        Math::Transpose(Math::Inverse(worldForNormal));

    if (resource_->transformationData_) {
        *resource_->transformationData_ = {
            resource_->transformationMatrix_.WVP,
            resource_->transformationMatrix_.world,
            resource_->transformationMatrix_.WorldInverseTranspose
        };
    }

    resource_->materialData_->uvTransform =
        Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

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

    if (drawManager_) {
        drawManager_->DrawObject3D(resource_->vertexBufferView_, resource_->indexBufferView_, resource_->materialResource_, resource_->transformationResource_, resource_->textureHandle_, resource_->indexCount_);
    }
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