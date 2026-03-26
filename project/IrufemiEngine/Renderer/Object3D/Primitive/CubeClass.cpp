#define NOMINMAX
#include "Renderer/Object3D/Primitive/CubeClass.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <Windows.h>

#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Engine/Manager/PrimitiveManager.h"

TextureManager* CubeClass::textureManager_ = nullptr;
DrawManager* CubeClass::drawManager_ = nullptr;
DebugUI* CubeClass::ui_ = nullptr;

// depth を受け取る本体実装
void CubeClass::Initialize(Camera* camera, float width, float height, float depth, const std::string& textureName) {
    camera_ = camera;
    width_ = width;
    height_ = height;
    depth_ = depth;

    // D3D12ResourceUtil の生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // サイズ反映(中心原点)
    const float hx = width_ * 0.5f;
    const float hy = height_ * 0.5f;
    const float hz = depth_ * 0.5f;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Cube);

    // リソース割当・データ転送（定数バッファのみ）
    resource_->CreateResource(); // Vertex/Index 以外を生成するために必要だが、現状の実装に合わせる
    resource_->Map();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // マテリアル初期化
    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = true;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();
    resource_->materialData_->shininess = 64.0f;

    // transformation
    resource_->transform_.translate = center_;
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };
    Vector3 effectiveScale{ 1.0f,1.0f,1.0f };
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = worldForNormal.m[3][1] = worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    // テクスチャハンドル設定
    if (textureManager_) {
        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        if (!textureNames.empty()) {
            resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);
            auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
            if (it != textureNames.end()) selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
            else selectedTextureIndex_ = 0;
        } else {
            resource_->textureHandle_.ptr = 0;
        }
    }

    if (resource_->textureHandle_.ptr == 0) {
        resource_->materialData_->hasTexture = false;
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
    // 位置/回転/スケールに応じてワールド行列を再計算する
    resource_->transform_.translate = center_;
    Vector3 effectiveScale{ 1.0f, 1.0f, 1.0f };
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = worldForNormal.m[3][1] = worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    if (resource_->transformationData_) {
        *resource_->transformationData_ = {
            resource_->transformationMatrix_.WVP,
            resource_->transformationMatrix_.world,
            resource_->transformationMatrix_.WorldInverseTranspose
        };
    }

    if (resource_->textureHandle_.ptr == 0) {
        resource_->materialData_->hasTexture = false;
    }

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void CubeClass::Draw() {
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

void CubeClass::Debug(const char* cubeName) {
#if defined USE_IMGUI
    std::string name = std::string("Cube: ") + cubeName;
    ImGui::Begin(name.c_str());

    // Transform / Material / Texture
    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

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
            auto textureNames = textureManager_->GetTextureNames();
            std::sort(textureNames.begin(), textureNames.end());
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