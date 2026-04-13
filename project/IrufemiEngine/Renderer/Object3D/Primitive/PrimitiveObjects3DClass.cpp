#include "PrimitiveObjects3DClass.h"

#include <algorithm>

#include "Engine/Manager/PrimitiveManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Application/camera/Camera.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Shape/Sphere.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

// 静的メンバの初期化
TextureManager* PrimitiveObjects3DClass::textureManager_ = nullptr;
DrawManager* PrimitiveObjects3DClass::drawManager_ = nullptr;
DebugUI* PrimitiveObjects3DClass::ui_ = nullptr;

// --- TransformComponent ---

void PrimitiveObjects3DClass::TransformComponent::UpdateTransform(Object3DResource* resource, const Camera& camera) {
    if (!resource) return;

    // 行列の更新
    // 既存の Object3DResource::UpdateTransform は内部で world 行列を再計算するため、
    // ここでは Transform の値を resource に同期させるだけで済む
    resource->transform_ = transform;
    resource->UpdateTransform(camera);

    isDirty = false;
}

// --- MeshComponent ---

void PrimitiveObjects3DClass::MeshComponent::ChangeMesh(PrimitiveType newType) {
    type = newType;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(type);

    if (!resource) {
        resource = std::make_unique<Object3DResource>();
    }

    // リソースの共有設定
    resource->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource->indexBufferView_ = primitiveResource.indexBufferView;
    resource->indexCount_ = primitiveResource.indexCount;

    // 定数バッファ等の生成
    resource->CreateResource();
    resource->Map();
}

// --- MaterialComponent ---

void PrimitiveObjects3DClass::MaterialComponent::UpdateMaterial(Object3DResource* resource, TextureManager* textureManager) {
    if (!resource || !resource->materialData_) return;

    // マテリアルパラメータの反映
    resource->materialData_->color = color;
    resource->materialData_->enableLighting = enableLighting;
    resource->materialData_->lightingMode = lightingMode;
    resource->materialData_->metallic = metallic;
    resource->materialData_->roughness = roughness;
    resource->materialData_->hasTexture = !texturePath.empty();

    // テクスチャハンドルの更新
    if (textureManager && !texturePath.empty()) {
        resource->textureHandle_ = textureManager->GetTextureHandle(texturePath);
        
        // ハンドルが取得できなかった場合はテクスチャ無効にする
        if (resource->textureHandle_.ptr == 0) {
            resource->materialData_->hasTexture = false;
        }
    } else {
        resource->materialData_->hasTexture = false;
    }
}

// --- PrimitiveObjects3DClass ---

void PrimitiveObjects3DClass::Initialize(Camera* camera, PrimitiveType type, const std::string& texturePath) {
    camera_ = camera;
    
    // 形状の初期化
    mesh_.ChangeMesh(type);

    // マテリアルの初期化
    material_.texturePath = texturePath;
    if (textureManager_) {
        // 現在のテクスチャ名からインデックスを復元（Debug UI用）
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), texturePath);
        material_.selectedTextureIndex = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }

    // デフォルトのマテリアル設定反映
    material_.UpdateMaterial(mesh_.resource.get(), textureManager_);

    // 初回のトランスフォーム更新
    transform_.isDirty = true;
    Update();
}

void PrimitiveObjects3DClass::Update() {
    if (!mesh_.resource || !camera_) return;

    // 必要に応じてトランスフォーム更新
    if (transform_.isDirty) {
        transform_.UpdateTransform(mesh_.resource.get(), *camera_);
    } else {
        // カメラが動いている可能性を考慮して常に更新（最適化が必要ならフラグ管理を厳密にする）
        mesh_.resource->UpdateTransform(*camera_);
    }

    // マテリアル情報の最新化
    material_.UpdateMaterial(mesh_.resource.get(), textureManager_);
}

void PrimitiveObjects3DClass::Draw() {
    if (camera_) {
        Draw(*camera_);
    }
}

void PrimitiveObjects3DClass::Draw(const Camera& camera) {
    if (!mesh_.resource || !drawManager_) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 形状に応じた基本半径（ユニットサイズ1.0想定）
        float baseRadius = 0.5f;
        switch (mesh_.type) {
        case PrimitiveType::Cube:      baseRadius = 0.866f; break; // 1/2 * sqrt(3)
        case PrimitiveType::Cylinder:
        case PrimitiveType::Cone:
        case PrimitiveType::Plane:
        case PrimitiveType::Triangle:
        case PrimitiveType::Tetra:     baseRadius = 0.707f; break; // 1/2 * sqrt(2)
        default:                       baseRadius = 0.500f; break;
        }

        // スケールを考慮した最終半径（異方性スケールの最大値を採用）
        float maxScale = (std::max)({ transform_.transform.scale.x, transform_.transform.scale.y, transform_.transform.scale.z });
        float finalRadius = baseRadius * maxScale;

        Sphere boundingSphere;
        boundingSphere.center = transform_.transform.translate;
        boundingSphere.radius = finalRadius * 1.1f; // 10%のマージン

        if (!Collision::IsCollision(camera.GetFrustum(), boundingSphere)) {
            return; // 描画スキップ
        }
    }

    // 描画実行
    drawManager_->DrawStandard3D(mesh_.resource.get());
}

void PrimitiveObjects3DClass::Debug(const char* label) {
#ifdef USE_IMGUI
    if (!ui_) return;

    ImGui::Begin(label);

    // --- Mesh Component ---
    if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* shapeNames[] = {
            "Triangle", "Plane", "Cube", "Cylinder", "Sphere", 
            "Tetra", "Circle", "Ring", "Skybox", "Cone", 
            "Torus", "IcoSphere", "Grid"
        };
        int currentType = static_cast<int>(mesh_.type);
        if (ImGui::Combo("Shape", &currentType, shapeNames, IM_ARRAYSIZE(shapeNames))) {
            SetShape(static_cast<PrimitiveType>(currentType));
        }
    }

    // --- Transform Component ---
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::DragFloat3("Position", &transform_.transform.translate.x, 0.01f)) transform_.isDirty = true;
        if (ImGui::DragFloat3("Rotation", &transform_.transform.rotate.x, 0.01f)) transform_.isDirty = true;
        if (ImGui::DragFloat3("Scale", &transform_.transform.scale.x, 0.01f)) transform_.isDirty = true;
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
    }

    // --- Material Component ---
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit4("Base Color", &material_.color.x);
        
        const char* lightingModes[] = { "None", "Lambert", "Half-Lambert", "PBR" };
        ImGui::Combo("Lighting Mode", &material_.lightingMode, lightingModes, IM_ARRAYSIZE(lightingModes));
        
        ImGui::Checkbox("Enable Lighting", &material_.enableLighting);
        
        if (material_.lightingMode == 3) { // PBR
            ImGui::SliderFloat("Metallic", &material_.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &material_.roughness, 0.0f, 1.0f);
        }

        // Texture 選択 (DebugUIの機能を利用)
        if (textureManager_) {
            ui_->DebugTexture(mesh_.resource.get(), material_.selectedTextureIndex);
            // 選択されたインデックスから名前を更新
            auto textureNames = textureManager_->GetTextureNamesForDebug();
            if (material_.selectedTextureIndex >= 0 && material_.selectedTextureIndex < static_cast<int>(textureNames.size())) {
                material_.texturePath = textureNames[material_.selectedTextureIndex];
            }
        }
    }

    ImGui::End();
#endif
}
