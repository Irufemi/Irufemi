#include "TriangleClass.h"
#include "Engine/Graphics/Camera/Camera.h"
#include <wrl.h>
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Math.h"
#include <string>
#include <algorithm>

TextureManager* TriangleClass::textureManager_ = nullptr;
DrawManager* TriangleClass::drawManager_ = nullptr;
DebugUI* TriangleClass::ui_ = nullptr;
IrufemiEngine* TriangleClass::engine_ = nullptr;

#include "Engine/IrufemiEngine.h"

void TriangleClass::Initialize(const std::string& textureName) {
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

    if (engine_) {
        if (Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera()) {
            resource_->UpdateTransform(*activeCam);
        }
    }

    // マテリアル初期設定
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

    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        
        // デバッグUI用のインデックス更新
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }
}

void TriangleClass::Update() {
    if (!resource_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 行列更新
    resource_->UpdateTransform(*activeCam);
    
    isDirty_ = false;
    lastViewMatrix_ = activeCam->GetViewMatrix();
    lastProjectionMatrix_ = activeCam->GetPerspectiveFovMatrix();
}

void TriangleClass::Draw() {
    if (!resource_ || !drawManager_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 単位正方形に収まる三角形を想定し、最大スケールから半径を算出
        float maxScale = (std::max)({ resource_->transform_.scale.x, resource_->transform_.scale.y, resource_->transform_.scale.z });
        float finalRadius = 0.7071f * maxScale; // 0.5 * sqrt(2)

        Sphere boundingSphere;
        boundingSphere.center = resource_->transform_.translate;
        boundingSphere.radius = finalRadius * 1.1f; // 10%のマージン

        if (!Collision::IsCollision(activeCam->GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &activeCam->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &activeCam->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    // --- 【追加】描画直前のバッファ同期 ---
    resource_->SyncBeforeDraw();

    // 共有リソースを使用して描画
    drawManager_->SubmitStandard3D(resource_.get(), nullptr, castShadows_);
}

void TriangleClass::Debug(const char* triangleName) {
#if defined USE_IMGUI
    std::string name = std::string("Triangle: ") + triangleName;
    ImGui::Begin(name.c_str());
    if (ui_) {
        ui_->DebugTransform(resource_->transform_);
        ui_->DebugMaterialBy3D(resource_->GetMaterialData());
        ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
        ui_->DebugUvTransform(resource_->uvTransform_);
    }
    ImGui::End();
#endif
    Update();
}

void TriangleClass::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}

