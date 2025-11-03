#include "CylinderClass.h"

#include <cmath>
#include "externals/imgui/imgui.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"

#include "function/Function.h"
#include "function/Math.h"
#include <string>
#include <algorithm>

TextureManager* CylinderClass::textureManager_ = nullptr;
DrawManager*    CylinderClass::drawManager_ = nullptr;
DebugUI*        CylinderClass::ui_ = nullptr;

// 上下面キャップを追加するヘルパー実装
void CylinderClass::AddCap(bool top, bool doubleSided) {
    // y と法線方向を決める
    float y = top ? 0.5f : -0.5f;
    Vector3 normal = top ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 0.0f, -1.0f, 0.0f };

    // 中心頂点
    uint32_t centerIndex = static_cast<uint32_t>(resource_->vertexDataList_.size());
    resource_->vertexDataList_.push_back({
        { 0.0f, y, 0.0f, 1.0f },
        { 0.5f, 0.5f }
    });
    resource_->vertexDataList_.back().normal = normal;

    // リム頂点（kSubdivision_ 周）
    uint32_t rimStart = static_cast<uint32_t>(resource_->vertexDataList_.size());
    for (uint32_t i = 0; i <= kSubdivision_; ++i) {
        float theta = i * kThetaEvery_;
        float cx = std::cos(theta);
        float sz = std::sin(theta);
        // UV は円盤マッピング（V 反転に合わせて 1 - で調整）
        float u = cx * 0.5f + 0.5f;
        float v = 1.0f - (sz * 0.5f + 0.5f);
        resource_->vertexDataList_.push_back({
            { cx, y, sz, 1.0f },
            { u, v }
        });
        resource_->vertexDataList_.back().normal = normal;
    }

    // インデックス（ファン）
    // ワインディングはレンダラ側のフロント定義に合わせるため、
    // 元コードと同様に rimStart+i+1 / rimStart+i の形にしてある（表向き）。
    for (uint32_t i = 0; i < kSubdivision_; ++i) {
        uint32_t v0 = centerIndex;
        uint32_t v1 = rimStart + i + 1;
        uint32_t v2 = rimStart + i;

        // 表向き三角形
        resource_->indexDataList_.push_back(v0);
        resource_->indexDataList_.push_back(v1);
        resource_->indexDataList_.push_back(v2);

        if (doubleSided) {
            // 逆順も追加して両面にする
            resource_->indexDataList_.push_back(v0);
            resource_->indexDataList_.push_back(v2);
            resource_->indexDataList_.push_back(v1);
        }
    }
}

// 初期化
void CylinderClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // D3D12ResourceUtil の生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // =========================
    // メッシュ生成（単位円柱）
    // 側面：半径=1, 高さ=1（y: -0.5 ～ +0.5）
    // 上下面：三角形ファン
    // =========================

    // 側面頂点
    for (uint32_t h = 0; h <= kHeightSubdivision_; ++h) {
        float t = static_cast<float>(h) / static_cast<float>(kHeightSubdivision_);
        float y = -0.5f + t * 1.0f; // -0.5 ～ +0.5
        for (uint32_t i = 0; i <= kSubdivision_; ++i) {
            float theta = i * kThetaEvery_;
            float cx = std::cos(theta);
            float sz = std::sin(theta);

            // 頂点
            resource_->vertexDataList_.push_back({
                { cx, y, sz, 1.0f },  // position
                { static_cast<float>(i) / static_cast<float>(kSubdivision_), 1.0f - t } // texcoord (u:角度, v:高さ)
            });
        }
    }

    // 側面法線を設定（y=0 の放射方向）
    for (uint32_t h = 0; h <= kHeightSubdivision_; ++h) {
        for (uint32_t i = 0; i <= kSubdivision_; ++i) {
            uint32_t idx = h * (kSubdivision_ + 1) + i;
            float theta = i * kThetaEvery_;
            float cx = std::cos(theta);
            float sz = std::sin(theta);
            resource_->vertexDataList_[idx].normal = { cx, 0.0f, sz };
        }
    }

    // 側面インデックス
    for (uint32_t h = 0; h < kHeightSubdivision_; ++h) {
        for (uint32_t i = 0; i < kSubdivision_; ++i) {
            uint32_t row = (kSubdivision_ + 1);
            uint32_t a = row * h + i;
            uint32_t b = row * (h + 1) + i;
            uint32_t c = row * h + (i + 1);
            uint32_t d = row * (h + 1) + (i + 1);

            resource_->indexDataList_.push_back(a);
            resource_->indexDataList_.push_back(b);
            resource_->indexDataList_.push_back(c);

            resource_->indexDataList_.push_back(b);
            resource_->indexDataList_.push_back(d);
            resource_->indexDataList_.push_back(c);
        }
    }

    // 上蓋を追加（通常は single-sided）
    AddCap(true /*top*/, false /*doubleSided*/);

    // 下蓋を追加（通常は single-sided）
    AddCap(false /*top*/, true /*doubleSided*/);

    // ========= リソース確保と書き込み =========

    resource_->CreateResource();
    resource_->Map();

    // 頂点バッファビュー
    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    // インデックスバッファビュー
    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(resource_->indexDataList_.size());
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);

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
        info_.radius * resource_->transform_.scale.x,
        info_.height * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
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

    // ライトとカメラ
    resource_->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
    resource_->directionalLightData_->intensity = 1.0f;

    resource_->cameraData_->worldPosition = camera_->GetTranslate();
}

void CylinderClass::Update(const char* cylinderName) {

#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Cylinder: ") + cylinderName;
    ImGui::Begin(name.c_str());

    // 中心・半径・高さの編集
    ImGui::DragFloat3("Center", &info_.center.x, 0.01f);
    ImGui::DragFloat("Radius", &info_.radius, 0.01f, 0.001f, 1000.0f);
    ImGui::DragFloat("Height", &info_.height, 0.01f, 0.001f, 1000.0f);

    // Transform（係数スケール・回転・位置）
    resource_->transform_.translate = info_.center;
    ui_->DebugTransform(resource_->transform_);

    // 位置を CylinderInfo に反映
    info_.center = resource_->transform_.translate;

    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);
    ui_->DebugDirectionalLight(resource_->directionalLightData_);

    ImGui::End();
#endif


    // Release でも必ず論理情報を実トランスフォームに反映する
    resource_->transform_.translate = info_.center;

    // 実スケール = { radius*scale.x, height*scale.y, radius*scale.z }
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.height * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);

    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world,
            Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線用（平行移動除去）
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

    resource_->materialData_->uvTransform =
        Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    resource_->directionalLightData_->direction =
        Math::Normalize(resource_->directionalLightData_->direction);

    resource_->cameraData_->worldPosition = camera_->GetTranslate();
}

void CylinderClass::Draw() {
    // DrawManager 側に DrawCylinder(CylinderClass*) を用意してください
    drawManager_->DrawCylinder(this);
}
