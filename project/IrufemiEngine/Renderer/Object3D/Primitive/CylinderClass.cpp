#include "Renderer/Object3D/Primitive/CylinderClass.h"

#include <cmath>
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"

#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Core/Shape/Sphere.h"
#include <string>
#include <algorithm>
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"

TextureManager* CylinderClass::textureManager_ = nullptr;
DrawManager* CylinderClass::drawManager_ = nullptr;
DebugUI* CylinderClass::ui_ = nullptr;

void CylinderClass::Initialize(Camera* camera, bool hasTop, bool hasBottom, const std::string& textureName) {
    this->camera_ = camera;
    this->hasTop_ = hasTop;
    this->hasBottom_ = hasBottom;
    // PrimitiveManager から指定されたパターンのリソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetCylinderResource(hasTop, hasBottom);

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
    if (resource_->GetMaterialData()) {
        resource_->GetMaterialData()->color = { 1.0f,1.0f,1.0f,1.0f };
        resource_->GetMaterialData()->enableLighting = true;
        resource_->GetMaterialData()->hasTexture = true;
        resource_->GetMaterialData()->lightingMode = 3;
        resource_->GetMaterialData()->uvTransform = Math::MakeIdentity4x4();
        resource_->GetMaterialData()->metallic = 0.0f;
        resource_->GetMaterialData()->roughness = 0.5f;
        resource_->GetMaterialData()->environmentCoefficient = 0.0f;
        resource_->GetMaterialData()->alphaReference = 0.5f;
        resource_->GetMaterialData()->useClampSampler = 0;
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

    if (resource_->GetMaterialData()) {
        resource_->GetMaterialData()->uvTransform =
            Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
        
        if (resource_->textureHandle_.ptr == 0) {
            resource_->GetMaterialData()->hasTexture = false;
        }
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void CylinderClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 半径(XZ)と高さ(Y)のスケールを考慮
        float rx = info_.radius * resource_->transform_.scale.x;
        float rz = info_.radius * resource_->transform_.scale.z;
        float ry_half = (info_.height * 0.5f) * resource_->transform_.scale.y;
        float horizontalMax = (std::max)(rx, rz);
        float finalRadius = (std::sqrt)(horizontalMax * horizontalMax + ry_half * ry_half);

        Sphere boundingSphere;
        boundingSphere.center = info_.center;
        boundingSphere.radius = finalRadius * 1.1f; // 10%のマージン

        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    // --- 【追加】描画直前のバッファ同期 ---
    resource_->SyncBeforeDraw();

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

    ui_->DebugMaterialBy3D(resource_->GetMaterialData());
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();
#endif
}

Vector3 CylinderClass::GetRight() const {
    if (!resource_) return { 1, 0, 0 };
    Matrix4x4 mat = Math::MakeRotateXYZMatrix(resource_->transform_.rotate);
    return { mat.m[0][0], mat.m[0][1], mat.m[0][2] };
}

Vector3 CylinderClass::GetUp() const {
    if (!resource_) return { 0, 1, 0 };
    Matrix4x4 mat = Math::MakeRotateXYZMatrix(resource_->transform_.rotate);
    return { mat.m[1][0], mat.m[1][1], mat.m[1][2] };
}

Vector3 CylinderClass::GetDirection() const {
    if (!resource_) return { 0, 0, 1 };
    Matrix4x4 mat = Math::MakeRotateXYZMatrix(resource_->transform_.rotate);
    return { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
}

void CylinderClass::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}

