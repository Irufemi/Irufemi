#include "Renderer/Object3D/Primitive/PlaneClass.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Texture/TextureManager.h"

#include <algorithm>
#include <array>

TextureManager* PlaneClass::textureManager_ = nullptr;
DrawManager* PlaneClass::drawManager_ = nullptr;
DebugUI* PlaneClass::ui_ = nullptr;
void PlaneClass::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;

    // PrimitiveManager から標準リソースを取得
    const auto& primitiveResource = PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Plane);

    // D3D12ResourceUtilを生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // 共有バッファの View とインデックス数を設定
    resource_->vertexBufferView_ = primitiveResource.vertexBufferView;
    resource_->indexBufferView_ = primitiveResource.indexBufferView;
    resource_->indexCount_ = primitiveResource.indexCount;

    // リソースのメモリを確保 (定数バッファのみ)
    resource_->CreateResource();
    resource_->Map();

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

    // 初期の平面情報(ワールド空間)も一度更新
    {
        // 法線は(0,0,-1)をWorldInverseTransposeで変換して正規化
        Vector3 nLocal{ 0.0f, 0.0f, -1.0f };
        // (x,y,z,0) として扱う
        Vector3 nWorld{
            nLocal.x * worldForNormal.m[0][0] + nLocal.y * worldForNormal.m[1][0] + nLocal.z * worldForNormal.m[2][0],
            nLocal.x * worldForNormal.m[0][1] + nLocal.y * worldForNormal.m[1][1] + nLocal.z * worldForNormal.m[2][1],
            nLocal.x * worldForNormal.m[0][2] + nLocal.y * worldForNormal.m[1][2] + nLocal.z * worldForNormal.m[2][2]
        };
        nWorld = Math::Normalize(nWorld);

        // 平面上の1点(ローカル原点が平面上)をワールドへ
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
    if (resource_->transformationData_) {
        *resource_->transformationData_ = {
            resource_->transformationMatrix_.WVP,
            resource_->transformationMatrix_.world,
            resource_->transformationMatrix_.WorldInverseTranspose
        };
    }

    // UV行列
    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    // 平面情報をワールド空間で更新
    {
        Vector3 nLocal{ 0.0f, 0.0f, -1.0f };
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

    // フラグ更新
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void PlaneClass::Draw() {
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

void PlaneClass::Debug([[maybe_unused]] const char* planeName) {
#if defined USE_IMGUI
    std::string name = std::string("Plane: ") + planeName;

    ImGui::Begin(name.c_str());

    ui_->DebugTransform(resource_->transform_);
    ui_->DebugMaterialBy3D(resource_->materialData_);
    ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
    ui_->DebugUvTransform(resource_->uvTransform_);

    ImGui::End();
#endif
}