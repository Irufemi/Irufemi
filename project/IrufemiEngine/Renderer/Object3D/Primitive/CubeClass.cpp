#define NOMINMAX
#include "CubeClass.h"

#include <algorithm>
#include <string>
#include <cstdio>
#include <Windows.h>

#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Geometry/Math.h"

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

    // 頂点データ(24頂点)
    resource_->vertexDataList_ = {
        // 前面 (-Z)
        { { -hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 0
        { { -hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 1
        { {  hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }, // 2
        { {  hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }, // 3
        // 背面 (+Z)
        { {  hx, -hy,  hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, // 4
        { {  hx,  hy,  hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // 5
        { { -hx, -hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } }, // 6
        { { -hx,  hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // 7
        // 左面 (-X)
        { { -hx, -hy,  hz, 1.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } }, // 8
        { { -hx,  hy,  hz, 1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // 9
        { { -hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f } }, // 10
        { { -hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f } }, // 11
        // 右面 (+X)
        { {  hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // 12
        { {  hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // 13
        { {  hx, -hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }, // 14
        { {  hx,  hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // 15
        // 下面 (-Y)
        { { -hx, -hy, -hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } }, // 16
        { {  hx, -hy, -hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f } }, // 17
        { { -hx, -hy,  hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // 18
        { {  hx, -hy,  hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f } }, // 19
        // 上面 (+Y)
        { { -hx,  hy,  hz, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, // 20
        { {  hx,  hy,  hz, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }, // 21
        { { -hx,  hy, -hz, 1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // 22
        { {  hx,  hy, -hz, 1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }  // 23
    };

    // インデックスデータ
    resource_->indexDataList_ = {
        0, 1, 2, 2, 1, 3, // 前面
        4, 5, 6, 6, 5, 7, // 背面
        8, 9, 10, 10, 9, 11, // 左面
        12, 13, 14, 14, 13, 15, // 右面
        16, 17, 18, 18, 17, 19, // 下面
        20, 21, 22, 22, 21, 23  // 上面
    };

    // リソース割当・データ転送
    resource_->CreateResource();
    resource_->Map();

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

    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    if (resource_->textureHandle_.ptr == 0) {
        resource_->materialData_->hasTexture = false;
    }

}

void CubeClass::Draw() {
    if (drawManager_) {
        drawManager_->DrawObject3D(resource_->vertexBufferView_, resource_->indexBufferView_, resource_->materialResource_, resource_->transformationResource_, resource_->textureHandle_, static_cast<UINT>(resource_->indexDataList_.size()));
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