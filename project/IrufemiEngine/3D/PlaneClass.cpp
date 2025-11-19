#include "PlaneClass.h"
#include "function/Math.h"
#include "function/Function.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include <imgui.h>

#include <algorithm>
#include <array>

TextureManager* PlaneClass::textureManager_ = nullptr;
DrawManager* PlaneClass::drawManager_ = nullptr;
DebugUI* PlaneClass::ui_ = nullptr;

void PlaneClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // D3D12ResourceUtilを生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // ローカルXZ平面上の4頂点（単位サイズ、中心原点）
    //  v3(-0.5,0.5,0)----v2(0.5,0.5,0)
    //      |                |
    //      |                |
    //  v0(-0.5,-0.5,0)--v1(0.5,-0.5,0)
    resource_->vertexDataList_.push_back({ { -0.5f, -0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }); // v0
    resource_->vertexDataList_.push_back({ {  0.5f,-0.5f, 0.0f,  1.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f } }); // v1
    resource_->vertexDataList_.push_back({ {  0.5f,  0.5f,0.0f,  1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }); // v2
    resource_->vertexDataList_.push_back({ { -0.5f,  0.5f,0.0f,  1.0f }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }); // v3

    // 2トライアングル
    resource_->indexDataList_.push_back(0);
    resource_->indexDataList_.push_back(1);
    resource_->indexDataList_.push_back(2);
    resource_->indexDataList_.push_back(0);
    resource_->indexDataList_.push_back(2);
    resource_->indexDataList_.push_back(3);

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
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

    // WVP
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線用：平行移動を除いたWorld
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 逆転置行列
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    // 定数バッファへ書き込み
    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    // テクスチャ設定
    auto textureNames = textureManager_->GetTextureNames();
    std::sort(textureNames.begin(), textureNames.end());
    if (!textureNames.empty()) {
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);

        auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
        if (it != textureNames.end()) {
            selectedTextureIndex_ = static_cast<int>(std::distance(textureNames.begin(), it));
        } else {
            selectedTextureIndex_ = 0;
        }
    }

    // ライト/カメラ
    resource_->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
    resource_->directionalLightData_->intensity = 1.0f;

    resource_->cameraData_->worldPosition = camera_->GetTranslate();

    // 初期の平面情報（ワールド空間）も一度更新
    {
        // 法線は(0,1,0)をWorldInverseTransposeで変換して正規化
        Vector3 nLocal{ 0.0f, 1.0f, 0.0f };
        // (x,y,z,0) として扱う
        Vector3 nWorld{
            nLocal.x * worldForNormal.m[0][0] + nLocal.y * worldForNormal.m[1][0] + nLocal.z * worldForNormal.m[2][0],
            nLocal.x * worldForNormal.m[0][1] + nLocal.y * worldForNormal.m[1][1] + nLocal.z * worldForNormal.m[2][1],
            nLocal.x * worldForNormal.m[0][2] + nLocal.y * worldForNormal.m[1][2] + nLocal.z * worldForNormal.m[2][2]
        };
        nWorld = Math::Normalize(nWorld);

        // 平面上の1点（ローカル原点が平面上）をワールドへ
        Vector3 pWorld{
            resource_->transformationMatrix_.world.m[3][0],
            resource_->transformationMatrix_.world.m[3][1],
            resource_->transformationMatrix_.world.m[3][2]
        };

        info_.normal = nWorld;
        info_.distance = (nWorld.x * pWorld.x) + (nWorld.y * pWorld.y) + (nWorld.z * pWorld.z);
    }
}

void PlaneClass::Update() {

    // Transform更新
    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, resource_->transform_.translate);
    resource_->transformationMatrix_.WVP = Math::Multiply(resource_->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // 法線用：平行移動を除いたWorld
    Matrix4x4 worldForNormal = resource_->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f;
    worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f;
    worldForNormal.m[3][3] = 1.0f;

    // 逆転置行列
    resource_->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    // Transform定数更新
    *resource_->transformationData_ = {
        resource_->transformationMatrix_.WVP,
        resource_->transformationMatrix_.world,
        resource_->transformationMatrix_.WorldInverseTranspose
    };

    // UV行列
    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    // ライト正規化
    resource_->directionalLightData_->direction = Math::Normalize(resource_->directionalLightData_->direction);

    // カメラ
    resource_->cameraData_->worldPosition = camera_->GetTranslate();

    // 平面情報をワールド空間で更新
    {
        Vector3 nLocal{ 0.0f, 1.0f, 0.0f };
        Vector3 nWorld{
            nLocal.x * worldForNormal.m[0][0] + nLocal.y * worldForNormal.m[1][0] + nLocal.z * worldForNormal.m[2][0],
            nLocal.x * worldForNormal.m[0][1] + nLocal.y * worldForNormal.m[1][1] + nLocal.z * worldForNormal.m[2][1],
            nLocal.x * worldForNormal.m[0][2] + nLocal.y * worldForNormal.m[1][2] + nLocal.z * worldForNormal.m[2][2]
        };
        nWorld = Math::Normalize(nWorld);

        Vector3 pWorld{
            resource_->transformationMatrix_.world.m[3][0],
            resource_->transformationMatrix_.world.m[3][1],
            resource_->transformationMatrix_.world.m[3][2]
        };

        info_.normal = nWorld;
        info_.distance = (nWorld.x * pWorld.x) + (nWorld.y * pWorld.y) + (nWorld.z * pWorld.z);
    }

}

void PlaneClass::Draw() {
    // インデックス付き描画
    drawManager_->DrawByIndex(resource_.get());
}

void PlaneClass::Debug([[maybe_unused]] const char* planeName) {
#if defined USE_IMGUI
    std::string name = std::string("Plane: ") + planeName;

    ImGui::Begin(name.c_str());

    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);
    ui_->DebugDirectionalLight(resource_->directionalLightData_);

    ImGui::End();
#endif
}