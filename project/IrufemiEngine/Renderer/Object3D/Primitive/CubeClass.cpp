#define NOMINMAX
#include "Renderer/Object3D/Primitive/CubeClass.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <Windows.h>

#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Engine/Core/Shape/Sphere.h"

TextureManager* CubeClass::textureManager_ = nullptr;
DrawManager* CubeClass::drawManager_ = nullptr;
DebugUI* CubeClass::ui_ = nullptr;

// depth を受け取る本体実装
void CubeClass::Initialize(Camera* camera, float width, float height, float depth, const std::string& textureName) {
    camera_ = camera;
    width_ = width;
    height_ = height;
    depth_ = depth;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Cube);

    // Object3DResource の生成
    resource_ = std::make_unique<Object3DResource>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // リソース割当・データ転送（定数バッファのみ）
    resource_->CreateResource();
    resource_->Map();

    // マテリアル初期化
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

    // transformation
    resource_->transform_.translate = center_;
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };
    
    Update();

    // テクスチャハンドル設定
    if (textureManager_) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        selectedTextureIndex_ = (it != textureNames.end()) ? static_cast<int>(std::distance(textureNames.begin(), it)) : 0;
    }

    if (resource_->textureHandle_.ptr == 0 && resource_->GetMaterialData()) {
        resource_->GetMaterialData()->hasTexture = false;
    }
}

void CubeClass::SetSize(float width, float height, float depth) {
    width_ = width;
    height_ = height;
    depth_ = depth;
    // サイズを変えた場合は再初期化が必要(頂点を再生成する方針)
    // 呼び出し側(Debug UI 等)で Initialize を呼ぶ設計にしているためここではリソース操作は行わない
}

void CubeClass::Update() {
    if (!resource_ || !camera_) return;

    // 位置/回転/スケールに応じてワールド行列を再計算する
    resource_->transform_.translate = center_;
    
    // 実スケール = 設定サイズ × 係数
    Vector3 effectiveScale{
        width_ * resource_->transform_.scale.x,
        height_ * resource_->transform_.scale.y,
        depth_ * resource_->transform_.scale.z
    };

    // 一時的にスケールを上書きして行列更新
    Vector3 originalScale = resource_->transform_.scale;
    resource_->transform_.scale = effectiveScale;
    resource_->UpdateTransform(*camera_);
    resource_->transform_.scale = originalScale;

    if (resource_->GetMaterialData()) {
        if (resource_->textureHandle_.ptr == 0) {
            resource_->GetMaterialData()->hasTexture = false;
        }
        resource_->GetMaterialData()->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void CubeClass::Draw() {
    if (!resource_ || !drawManager_ || !camera_) return;

    // 視錐台カリング
    if (isCullingEnabled_) {
        // 3軸サイズとスケールを考慮した境界球半径
        float rx = width_ * resource_->transform_.scale.x;
        float ry = height_ * resource_->transform_.scale.y;
        float rz = depth_ * resource_->transform_.scale.z;
        float finalRadius = 0.5f * (std::sqrt)(rx * rx + ry * ry + rz * rz);

        Sphere boundingSphere;
        boundingSphere.center = center_;
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

    drawManager_->SubmitStandard3D(resource_.get(), nullptr, castShadows_);
}

void CubeClass::Debug(const char* cubeName) {
#if defined USE_IMGUI
    std::string name = std::string("Cube: ") + cubeName;
    ImGui::Begin(name.c_str());

    // Transform / Material / Texture
    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->GetMaterialData());
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);

    // サイズ編集 UI(変更時は Initialize で再生成)
    float w = width_;
    float h = height_;
    float d = depth_;
    bool changed = false;
    if (ImGui::DragFloat("Width", &w, 0.01f, 0.0f, 1000.0f)) changed = true;
    if (ImGui::DragFloat("Height", &h, 0.01f, 0.0f, 1000.0f)) changed = true;
    if (ImGui::DragFloat("Depth", &d, 0.01f, 0.0f, 1000.0f)) changed = true;

    if (changed) {
        // 安全化
        w = std::max(0.0f, w);
        h = std::max(0.0f, h);
        d = std::max(0.0f, d);

        // 現在のテクスチャ名を復元して Initialize を呼ぶ(UI 保持のため)
        std::string currentTextureName = "resources/uvChecker.png";
        if (textureManager_) {
            auto textureNames = textureManager_->GetTextureNamesForDebug();
            if (!textureNames.empty()) {
                if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < static_cast<int>(textureNames.size())) {
                    currentTextureName = textureNames[selectedTextureIndex_];
                } else {
                    currentTextureName = textureNames[0];
                }
            }
        }

        // SetSize は内部値更新のみ。再生成は Initialize を呼ぶ
        SetSize(w, h, d);
        Initialize(camera_, w, h, d, currentTextureName);
    }

    ImGui::End();
#endif
}

void CubeClass::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}

