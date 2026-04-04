#include "SphereClass.h"

#include "Engine/Manager/PrimitiveManager.h"
#include <cmath>
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"

#include "Engine/Core/Math/Geometry/Math.h"
#include <string>
#include <algorithm>
#include <cstdio>
#include <Windows.h>

TextureManager* SphereClass::textureManager_ = nullptr;
DrawManager* SphereClass::drawManager_ = nullptr;
DebugUI* SphereClass::ui_ = nullptr;

//初期化
void SphereClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Sphere);

    // Object3DResourceの生成
    resource_ = std::make_unique<Object3DResource>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // メモリを確保 (定数バッファのみ生成するために必要)
    resource_->CreateResource();

    // 書き込みをできる状態にする
    resource_->Map();

    // 共有リソースを使用するため、個別のバッファへのコピーは不要

    //マテリアル
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

    //transformationMatrix
    resource_->transform_.translate = info_.center;
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };

    Update();

    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);

        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void SphereClass::Update() {
    if (!resource_ || !camera_) return;

    // Release でも必ず論理情報を実トランスフォームに反映する
    resource_->transform_.translate = info_.center;

    // 実スケール = 半径 × 係数
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.radius * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    // 一時的にスケールを上書きして行列更新
    Vector3 originalScale = resource_->transform_.scale;
    resource_->transform_.scale = effectiveScale;
    resource_->UpdateTransform(*camera_);
    resource_->transform_.scale = originalScale;

    // UV Transform 更新
    if (resource_->materialData_) {
        resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
        
        // SRVが無効なら毎フレーム保険で hasTexture をオフ
        if (resource_->textureHandle_.ptr == 0) {
            resource_->materialData_->hasTexture = false;
        }
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void SphereClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }

    drawManager_->DrawStandard3D(resource_.get());
}

void SphereClass::Debug([[maybe_unused]] const char* sphereName) {

#if defined USE_IMGUI

    std::string name = std::string("Sphere: ") + sphereName;

    ImGui::Begin(name.c_str());

    // 半径と中心は DebugSphereInfo で編集
    ui_->DebugSphereInfo(info_);

    // 位置は Transform 側でも編集されるので同期
    resource_->transform_.translate = info_.center;

    // scale は係数(1.0 基準)。ここで自由に非等方も可。
    ui_->DebugTransform(resource_->transform_);

    // 位置を SphereInfo に反映(半径は Transform からは変更しない)
    info_.center = resource_->transform_.translate;

    ui_->DebugMaterialBy3D(resource_->materialData_);

    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();

#endif // _DEBUG
}

