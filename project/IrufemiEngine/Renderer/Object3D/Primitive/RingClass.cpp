#define NOMINMAX
#include "Renderer/Object3D/Primitive/RingClass.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <Windows.h>

#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Shape/Sphere.h"

TextureManager* RingClass::textureManager_ = nullptr;
DrawManager* RingClass::drawManager_ = nullptr;
DebugUI* RingClass::ui_ = nullptr;
IrufemiEngine* RingClass::engine_ = nullptr;

#include "Engine/IrufemiEngine.h"

void RingClass::Initialize(const RingParams& params, const std::string& textureName) {
    ringParams_ = params;

    // リソースの確保と生成
    RebuildResource();

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

void RingClass::RebuildResource() {
    // 既存リソースのパラメータ保持用
    Transform oldTransform = { {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Material oldMaterial = {};
    D3D12_GPU_DESCRIPTOR_HANDLE oldTexHandle = {};
    if (resource_) {
        oldTransform = resource_->transform_;
        if (resource_->GetMaterialData()) {
            oldMaterial = *resource_->GetMaterialData();
        }
        oldTexHandle = resource_->textureHandle_;
    }

    // 古いリソースを破棄し、新しく確保
    resource_ = std::make_unique<Object3DResource>();
    
    // パラメータの復元
    resource_->transform_ = oldTransform;
    if (resource_->GetMaterialData()) {
        *resource_->GetMaterialData() = oldMaterial;
    }
    resource_->textureHandle_ = oldTexHandle;

    // PrimitiveData の生成
    PrimitiveData data = PrimitiveManager::GetInstance()->CreateRing(ringParams_);

    // Object3DResource へデータをコピー
    resource_->vertexDataList_ = data.vertices;
    resource_->indexDataList_ = data.indices;
    resource_->indexCount_ = static_cast<uint32_t>(data.indices.size());

    // リソース割当・データ転送
    resource_->CreateResource();
    resource_->Map();
    
    // 頂点データとインデックスデータのメモリコピー
    if (resource_->vertexData_ && !data.vertices.empty()) {
        std::copy(data.vertices.begin(), data.vertices.end(), resource_->vertexData_);
    }
    if (resource_->indexData_ && !data.indices.empty()) {
        std::copy(data.indices.begin(), data.indices.end(), resource_->indexData_);
    }

    isDirty_ = true;
}

void RingClass::Update() {
    if (!resource_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // 位置/回転/スケールに応じてワールド行列を再計算する
    resource_->transform_.translate = center_;
    
    resource_->UpdateTransform(*activeCam);

    if (resource_->GetMaterialData()) {
        if (resource_->textureHandle_.ptr == 0) {
            resource_->GetMaterialData()->hasTexture = false;
        }
        resource_->GetMaterialData()->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = activeCam->GetViewMatrix();
    lastProjectionMatrix_ = activeCam->GetPerspectiveFovMatrix();
}

void RingClass::Draw() {
    if (!resource_ || !drawManager_ || !engine_) return;
    Camera* activeCam = engine_->GetCameraManager()->GetActiveCamera();
    if (!activeCam) return;

    // カメラの行列が変更されたか、オブジェクト自体が変更されたかチェック
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &activeCam->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &activeCam->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    // 描画直前のバッファ同期
    SyncBeforeDraw();

    drawManager_->SubmitStandard3D(resource_.get(), nullptr, castShadows_);
}

void RingClass::Debug(const char* name) {
#if defined USE_IMGUI
    std::string windowName = std::string("Ring: ") + name;
    ImGui::Begin(windowName.c_str());

    // Transform / Material / Texture
    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->GetMaterialData());
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

    ImGui::Separator();
    ImGui::Text("Ring Parameters");

    bool changed = false;

    if (ImGui::DragFloat("Inner Radius", &ringParams_.innerRadius, 0.01f, 0.0f, 100.0f)) changed = true;
    if (ImGui::DragFloat("Start Outer Radius", &ringParams_.startOuterRadius, 0.01f, 0.0f, 100.0f)) changed = true;
    if (ImGui::DragFloat("End Outer Radius", &ringParams_.endOuterRadius, 0.01f, 0.0f, 100.0f)) changed = true;
    
    if (ImGui::DragFloat("Start Angle", &ringParams_.startAngle, 1.0f, 0.0f, 360.0f)) changed = true;
    if (ImGui::DragFloat("End Angle", &ringParams_.endAngle, 1.0f, 0.0f, 360.0f)) changed = true;
    
    // intにキャストしてスライダーで制御
    int segments = static_cast<int>(ringParams_.segments);
    if (ImGui::SliderInt("Segments", &segments, 3, 128)) {
        ringParams_.segments = static_cast<uint32_t>(segments);
        changed = true;
    }

    if (ImGui::Checkbox("Vertical UV", &ringParams_.verticalUV)) changed = true;

    ImGui::Separator();
    ImGui::Text("Color & Fade");
    
    float innerCol[4] = { ringParams_.innerColor.x, ringParams_.innerColor.y, ringParams_.innerColor.z, ringParams_.innerColor.w };
    if (ImGui::ColorEdit4("Inner Color", innerCol)) {
        ringParams_.innerColor = { innerCol[0], innerCol[1], innerCol[2], innerCol[3] };
        changed = true;
    }
    
    float outerCol[4] = { ringParams_.outerColor.x, ringParams_.outerColor.y, ringParams_.outerColor.z, ringParams_.outerColor.w };
    if (ImGui::ColorEdit4("Outer Color", outerCol)) {
        ringParams_.outerColor = { outerCol[0], outerCol[1], outerCol[2], outerCol[3] };
        changed = true;
    }

    if (ImGui::SliderFloat("Start Alpha", &ringParams_.startAlpha, 0.0f, 1.0f)) changed = true;
    if (ImGui::SliderFloat("End Alpha", &ringParams_.endAlpha, 0.0f, 1.0f)) changed = true;
    if (ImGui::DragFloat("Fade Range Angle", &ringParams_.fadeRangeAngle, 1.0f, 0.0f, 360.0f)) changed = true;

    if (changed) {
        // パラメータが変更されたらリソースを再構築
        RebuildResource();
    }

    ImGui::End();
#endif
}

void RingClass::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}
