#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"

#include <algorithm>
#include <array>

TextureManager* PlaneClass::textureManager_ = nullptr;
DrawManager* PlaneClass::drawManager_ = nullptr;
DebugUI* PlaneClass::ui_ = nullptr;
void PlaneClass::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Plane);

    // Object3DResourceを生成
    resource_ = std::make_unique<Object3DResource>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // リソースのメモリを確保 (定数バッファのみ)
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

    Update();

    // テクスチャ設定
    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void PlaneClass::Update() {
    if (!resource_ || !camera_) return;

    // Transform更新
    resource_->UpdateTransform(*camera_);

    // UV行列
    if (resource_->materialData_) {
        resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
    }

    // 平面情報をワールド空間で更新
    {
        // 平行移動を除いた行列
        Matrix4x4 m = resource_->transformationData_->world;
        m.m[3][0] = m.m[3][1] = m.m[3][2] = 0.0f;
        m.m[3][3] = 1.0f;

        Vector3 nLocal{ 0.0f, 0.0f, -1.0f };
        Vector3 nWorld{
            nLocal.x * m.m[0][0] + nLocal.y * m.m[1][0] + nLocal.z * m.m[2][0],
            nLocal.x * m.m[0][1] + nLocal.y * m.m[1][1] + nLocal.z * m.m[2][1],
            nLocal.x * m.m[0][2] + nLocal.y * m.m[1][2] + nLocal.z * m.m[2][2]
        };
        nWorld = Math::Normalize(nWorld);

        Vector3 pWorld{
            resource_->transformationData_->world.m[3][0],
            resource_->transformationData_->world.m[3][1],
            resource_->transformationData_->world.m[3][2]
        };

        info_.normal = nWorld;
        info_.distance = (nWorld.x * pWorld.x) + (nWorld.y * pWorld.y) + (nWorld.z * pWorld.z);
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void PlaneClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }

    drawManager_->DrawStandard3D(resource_.get());
}

void PlaneClass::Debug([[maybe_unused]] const char* planeName) {
#if defined USE_IMGUI
    std::string name = std::string("Plane: ") + planeName;

    ImGui::Begin(name.c_str());

    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();
#endif
}