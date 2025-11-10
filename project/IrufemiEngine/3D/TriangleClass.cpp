#include "TriangleClass.h"
#include "function/Math.h"
#include "function/Function.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include <imgui.h>

#include <algorithm>

TextureManager* TriangleClass::textureManager_ = nullptr;
DrawManager* TriangleClass::drawManager_ = nullptr;
DebugUI* TriangleClass::ui_ = nullptr;

void TriangleClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // D3D12ResourceUtilを生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    //　左下
    resource_->vertexDataList_.push_back({ { -0.5f,-0.5f,0.0f,1.0f }, { 0.0f,1.0f } });
    //　上
    resource_->vertexDataList_.push_back({ { 0.0f,0.5f,0.0f,1.0f  }, { 0.5f,0.0f } });
    //　右下
    resource_->vertexDataList_.push_back({ { 0.5f,-0.5f,0.0f,1.0f }, { 1.0f,1.0f } });

    for (uint32_t i = 0; i < static_cast<uint32_t>(resource_->vertexDataList_.size()); ++i) {
        resource_->vertexDataList_[i].normal.x = resource_->vertexDataList_[i].position.x;
        resource_->vertexDataList_[i].normal.y = resource_->vertexDataList_[i].position.y;
        resource_->vertexDataList_[i].normal.z = resource_->vertexDataList_[i].position.z;
    }

    resource_->indexDataList_.push_back(0); //　左下 
    resource_->indexDataList_.push_back(1); //　上
    resource_->indexDataList_.push_back(2); //　右下

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    //頂点バッファ

    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};

    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    //リソースの先頭のアドレスから使う
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    //使用するリソースのサイズ
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * static_cast<UINT>(resource_->indexDataList_.size());
    //インデックスはint32_tとする
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    ///IndexResourceにデータを書き込む

    //インデックスリソースにデータを書き込む

    std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);

    //マテリアル

    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = true;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();
    resource_->materialData_->shininess = 64.0f;

    //wvp

    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);

    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線変換用：平行移動を除いた World を使う
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 逆転置行列を計算
    resource_->transformationMatrix_.WorldInverseTranspose =
        Math::Transpose(Math::Inverse(worldForNormal));

    // 定数バッファへ全フィールドを書き込む
    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    auto textureNames = textureManager_->GetTextureNames();
    std::sort(textureNames.begin(), textureNames.end());
    if (!textureNames.empty()) {

        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);

        // コンボボックス用に selectedIndex を初期化
        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        if (it != textureNames.end()) {
            selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
        } else {
            selectedTextureIndex_ = 0;
        }

    }

    resource_->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
    resource_->directionalLightData_->intensity = 1.0f;

    resource_->cameraData_->worldPosition = camera_->GetTranslate();

}

void TriangleClass::Update(const char* triangleName) {

#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Triangle: ") + triangleName;

    //ImGui

    //カメラウィンドウを作り出す
    ImGui::Begin(name.c_str());

    ui_->DebugTransform(resource_->transform_);

    ui_->DebugMaterialBy3D(resource_->materialData_);

    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

    ui_->DebugUvTransform(resource_->uvTransform_);

    ui_->DebugDirectionalLight(resource_->directionalLightData_);

    //入力終了
    ImGui::End();

#endif // _DEBUG

    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線変換用：平行移動を除いた World を使う
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 逆転置行列を計算
    resource_->transformationMatrix_.WorldInverseTranspose =
        Math::Transpose(Math::Inverse(worldForNormal));

    // 定数バッファへ全フィールドを書き込む
    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    resource_->directionalLightData_->direction = Math::Normalize(resource_->directionalLightData_->direction);

    resource_->cameraData_->worldPosition = camera_->GetTranslate();
}

