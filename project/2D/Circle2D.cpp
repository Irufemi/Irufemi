#define NOMINMAX
#include "Circle2D.h"

#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "function/Math.h"
#include <imgui.h>
#include <algorithm>
#include <vector>

TextureManager* Circle2D::textureManager_ = nullptr;
DrawManager*    Circle2D::drawManager_ = nullptr;
DebugUI*        Circle2D::ui_ = nullptr;

void Circle2D::Initialize(Camera* camera, const std::string& textureName, uint32_t subdiv) {
    camera_ = camera;
    subdivision_ = std::max<uint32_t>(3, subdiv & ~1u); // 偶数に丸め、最低3

    resource_ = std::make_unique<D3D12ResourceUtil>();

    // 頂点/インデックス生成
    BuildUnitCircleFan(subdivision_);

    // GPUリソース確保
    resource_->CreateResource();
    resource_->Map();

    // VB/IBビュー
    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(resource_->indexDataList_.size());
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);

    // マテリアル/WVP 初期設定
    InitMaterialAndMatrix();

    // テクスチャ設定
    if (textureManager_) {
        auto names = textureManager_->GetTextureNames();
        std::sort(names.begin(), names.end());
        if (!names.empty()) {
            int idx = 0;
            if (!textureName.empty()) {
                auto it = std::find(names.begin(), names.end(), textureName);
                if (it != names.end()) idx = static_cast<int>(std::distance(names.begin(), it));
            }
            selectedTextureIndex_ = std::clamp(idx, 0, static_cast<int>(names.size()) - 1);
            resource_->textureHandle_ = textureManager_->GetTextureHandle(names[selectedTextureIndex_]);
        }
    }
    resource_->materialData_->hasTexture = useTexture_;
}

void Circle2D::BuildUnitCircleFan(uint32_t subdiv) {
    // 頂点データ: position(float4), tex(float2), normal(float3)
    resource_->vertexDataList_.clear();
    resource_->indexDataList_.clear();

    // 中心頂点（UVは中心）
    resource_->vertexDataList_.push_back({
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.5f, 0.5f },
        { 0.0f, 0.0f, -1.0f }
    });

    // 周上頂点
    const float step = 2.0f * pi_ / static_cast<float>(subdiv);
    for (uint32_t i = 0; i <= subdiv; ++i) {
        float th = step * static_cast<float>(i);
        float x = std::cos(th);
        float y = std::sin(th);
        // UVは[-1,1] -> [0,1] へ射影（V反転込み）
        float u = 0.5f + 0.5f * x;
        float v = 0.5f - 0.5f * y;

        resource_->vertexDataList_.push_back({
            { x, y, 0.0f, 1.0f },
            { u, v },
            { 0.0f, 0.0f, -1.0f }
        });
    }

    // インデックス（三角形ファン）
    // 中心: 0, 周: [1..subdiv+1]
    for (uint32_t i = 1; i <= subdiv; ++i) {
        resource_->indexDataList_.push_back(0);
        resource_->indexDataList_.push_back(i);
        resource_->indexDataList_.push_back(i + 1);
    }
}

void Circle2D::InitMaterialAndMatrix() {
    // transform 初期値
    // resource_->transform_.scale は「係数」として扱う（SphereClass と同様）
    resource_->transform_.scale = { 1.0f, 1.0f, 1.0f };
    resource_->transform_.rotate = { 0.0f, 0.0f, 0.0f };
    resource_->transform_.translate = info_.center;

    // 実スケール = 半径 × 係数（非等方スケールを許容）
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.radius * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    // 行列
    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP, resource_->transformationMatrix_.world };

    // マテリアル
    resource_->materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    resource_->materialData_->enableLighting = false;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();

    // ライト（未使用でも初期化）
    resource_->directionalLightData_->color = { 1,1,1,1 };
    resource_->directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
    resource_->directionalLightData_->intensity = 1.0f;
}

void Circle2D::UpdateMatrix() {
    // transform_.scale は係数のまま（非等方を許容）
    resource_->transform_.translate = info_.center;

    // 実スケール = 半径 × 係数（各成分個別に乗算）
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.radius * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP, resource_->transformationMatrix_.world };
}

void Circle2D::Update(const char* circleName) {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Circle2D: ") + circleName;
    ImGui::Begin(name.c_str());
    ui_->DebugTransform2D(resource_->transform_);
    ui_->DebugMaterialBy2D(resource_->materialData_);

    bool useTex = useTexture_;
    if (ImGui::Checkbox("UseTexture", &useTex)) {
        SetUseTexture(useTex);
    }

    if (textureManager_) {
        auto names = textureManager_->GetTextureNames();
        std::sort(names.begin(), names.end());
        if (!names.empty()) {
            const char* preview = names[selectedTextureIndex_].c_str();
            if (ImGui::BeginCombo("Texture", preview)) {
                for (int i = 0; i < static_cast<int>(names.size()); ++i) {
                    bool sel = (i == selectedTextureIndex_);
                    if (ImGui::Selectable(names[i].c_str(), sel)) {
                        selectedTextureIndex_ = i;
                        resource_->textureHandle_ = textureManager_->GetTextureHandle(names[i]);
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
    }
    ImGui::End();
#endif

    // 毎フレーム行列更新（カメラ正射影が動く可能性があるため）
    UpdateMatrix();

    // UV 変換はSpriteと同様の意味付け（ここではIdentityのまま）
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();

    // ライト正規化
    resource_->directionalLightData_->direction = Math::Normalize(resource_->directionalLightData_->direction);
}

void Circle2D::Draw() {
    drawManager_->DrawByIndex(resource_.get());
}

void Circle2D::SetInfo(const Circle2DInfo& info) {
    info_ = info;
    UpdateMatrix();
}

void Circle2D::SetCenter(const Vector3& center) {
    info_.center = center;
    UpdateMatrix();
}

void Circle2D::SetRadius(float radius) {
    info_.radius = radius;
    UpdateMatrix();
}

bool Circle2D::SetTextureByName(const std::string& textureName) {
    if (!textureManager_) return false;
    auto names = textureManager_->GetTextureNames();
    std::sort(names.begin(), names.end());
    auto it = std::find(names.begin(), names.end(), textureName);
    if (it == names.end()) return false;
    selectedTextureIndex_ = static_cast<int>(std::distance(names.begin(), it));
    resource_->textureHandle_ = textureManager_->GetTextureHandle(*it);
    return true;
}
