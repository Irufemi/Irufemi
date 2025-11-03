#include "SphereClass.h"

#include <cmath>
#include "externals/imgui/imgui.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"

#include "function/Function.h"
#include "function/Math.h"
#include <string>
#include <algorithm>
#include <cstdio>
#include <Windows.h>

TextureManager* SphereClass::textureManager_ = nullptr;
DrawManager* SphereClass::drawManager_ = nullptr;
DebugUI* SphereClass::ui_ = nullptr;

//初期化
void SphereClass::Initialize(Camera* camera, const std::string& textureName) {

    this->camera_ = camera;

    // D3D12ResourceUtilの生成
    resource_ = std::make_unique<D3D12ResourceUtil>();

    //上と下で頂点は重なるがtexcoordがおかしくなるので必要

    //緯度の方向に分割
    for (uint32_t latIndex = 0; latIndex <= kSubdivision_; ++latIndex) {
        float lat = -pi_ / 2.0f + kLatEvery_ * latIndex; //θ

        //経度の方向に分割しながら線を描く
        for (uint32_t lonIndex = 0; lonIndex <= kSubdivision_; ++lonIndex) {

            float lon = lonIndex * kLonEvery_; //φ

            //頂点にデータを入力する。基準点a

             //頂点リソースにデータを書き込む

            resource_->vertexDataList_.push_back( //左下
                {
                    {
                        std::cos(lat) * std::cos(lon),
                        std::sin(lat),
                        std::cos(lat) * std::sin(lon),
                        1.0f
                    },
                    {
                        static_cast<float>(lonIndex) / static_cast<float>(kSubdivision_),
                        1.0f - static_cast<float>(latIndex) / static_cast<float>(kSubdivision_)
                    }
                }
            );

        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(resource_->vertexDataList_.size()); ++i) {
        resource_->vertexDataList_[i].normal.x = resource_->vertexDataList_[i].position.x;
        resource_->vertexDataList_[i].normal.y = resource_->vertexDataList_[i].position.y;
        resource_->vertexDataList_[i].normal.z = resource_->vertexDataList_[i].position.z;
    }

    /*頂点インデックス*/

    for (uint32_t latIndex = 0; latIndex < kSubdivision_; ++latIndex) {
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision_; ++lonIndex) {
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * latIndex + lonIndex);
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * (latIndex + 1) + lonIndex);//左上
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * latIndex + (lonIndex + 1));//右下
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * (latIndex + 1) + lonIndex);//左上
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * (latIndex + 1) + (lonIndex + 1));//右上
            resource_->indexDataList_.push_back((kSubdivision_ + 1) * latIndex + (lonIndex + 1));//右下
        }
    }

    // メモリを確保
    resource_->CreateResource();

    // 書き込みをできる状態にする
    resource_->Map();

    //頂点バッファ

    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};

    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    /*頂点インデックス*/

    ///Index用のあれやこれやを作る

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

    //transformationMatrix

    resource_->transform_.translate = info_.center;

    // scaleは係数として扱う（初期値は等方1）
    resource_->transform_.scale = Vector3{ 1.0f,1.0f,1.0f };

    // 実スケール = 半径 × 係数
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.radius * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);

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

    } else {
        resource_->textureHandle_.ptr = 0;
    }

    // --- 追加: 実行時チェックとフォールバック ---
    if (resource_->textureHandle_.ptr == 0) {
        resource_->materialData_->hasTexture = false;
        OutputDebugStringA("[SphereClass] textureHandle is NULL -> using no-texture fallback\n");
    } else {
        resource_->materialData_->hasTexture = true;
        char buf[256];
        sprintf_s(buf, "[SphereClass] textureHandle.ptr = %llu\n", static_cast<unsigned long long>(resource_->textureHandle_.ptr));
        OutputDebugStringA(buf);
    }

    resource_->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
    resource_->directionalLightData_->intensity = 1.0f;

    resource_->cameraData_->worldPosition = camera_->GetTranslate();

}

void SphereClass::Update(bool debug,const char* sphereName) {

    if (debug) {
#if defined(_DEBUG) || defined(DEVELOPMENT)

        std::string name = std::string("Sphere: ") + sphereName;

        ImGui::Begin(name.c_str());

        // 半径と中心は DebugSphereInfo で編集
        ui_->DebugSphereInfo(info_);

        // 位置は Transform 側でも編集されるので同期
        resource_->transform_.translate = info_.center;

        // scale は係数（1.0 基準）。ここで自由に非等方も可。
        ui_->DebugTransform(resource_->transform_);

        // 位置を SphereInfo に反映（半径は Transform からは変更しない）
        info_.center = resource_->transform_.translate;

        ui_->DebugMaterialBy3D(resource_->materialData_);

        ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

        ui_->DebugUvTransform(resource_->uvTransform_);

        ui_->DebugDirectionalLight(resource_->directionalLightData_);

        ImGui::End();

#endif // _DEBUG

    }

    // Release でも必ず論理情報を実トランスフォームに反映する
    resource_->transform_.translate = info_.center;

    // 実スケール = 半径 × 係数
    Vector3 effectiveScale{
        info_.radius * resource_->transform_.scale.x,
        info_.radius * resource_->transform_.scale.y,
        info_.radius * resource_->transform_.scale.z
    };

    resource_->transformationMatrix_.world = Math::MakeAffineMatrix(effectiveScale, resource_->transform_.rotate, resource_->transform_.translate);

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

    // SRVが無効なら毎フレーム保険で hasTexture をオフ
    if (resource_->textureHandle_.ptr == 0) {
        resource_->materialData_->hasTexture = false;
    }

    resource_->materialData_->uvTransform = Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    resource_->directionalLightData_->direction = Math::Normalize(resource_->directionalLightData_->direction);

    resource_->cameraData_->worldPosition = camera_->GetTranslate();

}

void SphereClass::Draw() {
    drawManager_->DrawSphere(this);
}

