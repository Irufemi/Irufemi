#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h> // OutputDebugStringA を使う場合

#include "source/Texture.h"
#include "function/Function.h"
#include "function/Math.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "externals/imgui/imgui.h"
#include "engine/directX/DirectXCommon.h"

TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;

void ObjClass::Initialize(Camera* camera, const std::string& filename) {

    this->camera_ = camera;

    objModel_ = LoadObjFileM("resources/obj", filename);

    textures_.clear();
    resources_.clear();

    for (const auto& mesh : objModel_.meshes) {

        auto res = std::make_unique<D3D12ResourceUtil>();

        // 頂点バッファ
        res->vertexResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(VertexData) * mesh.vertices.size());
        res->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};
        res->vertexBufferView_.BufferLocation = res->vertexResource_->GetGPUVirtualAddress();
        res->vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * mesh.vertices.size());
        res->vertexBufferView_.StrideInBytes = sizeof(VertexData);

        res->vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->vertexData_));
        res->vertexDataList_ = mesh.vertices;
        std::memcpy(res->vertexData_, mesh.vertices.data(), sizeof(VertexData) * mesh.vertices.size());

        // マテリアル
        res->materialResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(Material));
        res->materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->materialData_));
        res->materialData_->color = mesh.material.color;
        res->materialData_->enableLighting = mesh.material.enableLighting;
        res->materialData_->hasTexture = true;
        res->materialData_->lightingMode = 2;
        res->materialData_->uvTransform = mesh.material.uvTransform; // すでに行列
        res->materialData_->shininess = 64.0f;

        // WVP
        res->transformationMatrix_.world = Math::MakeAffineMatrix(res->transform_.scale, res->transform_.rotate, res->transform_.translate);
        res->transformationMatrix_.WVP = Math::Multiply(res->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
        res->transformationResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(TransformationMatrix));
        res->transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->transformationData_));
        // 法線変換用：平行移動を除いた World を使う
        Matrix4x4 worldForNormal = res->transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f;
        worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f;
        worldForNormal.m[3][3] = 1.0f;

        // 逆転置行列を計算
        res->transformationMatrix_.WorldInverseTranspose =
            Math::Transpose(Math::Inverse(worldForNormal));

        // 定数バッファへ全フィールドを書き込む
        *res->transformationData_ = {
            res->transformationMatrix_.WVP,
            res->transformationMatrix_.world,
            res->transformationMatrix_.WorldInverseTranspose
        };

        // テクスチャ
        auto tex = std::make_unique<Texture>();
        if (!mesh.material.textureFilePath.empty()) {
            namespace fs = std::filesystem;
            std::string texStr = mesh.material.textureFilePath;

            // 区切り文字を統一（任意）
            std::replace(texStr.begin(), texStr.end(), '/', '\\');

            // ドライブ文字のみでスラッシュがない場合は補正: "F:foo.png" -> "F:\foo.png"
            if (texStr.size() >= 2 && texStr[1] == ':' && (texStr.size() == 2 || (texStr.size() > 2 && texStr[2] != '\\' && texStr[2] != '/'))) {
                texStr.insert(2, "\\");
            }

            fs::path texPath(texStr);

            // 既に absolute / resources/obj を含むなら prepend しない
            bool containsResourcesObj = (texStr.find("resources\\obj") != std::string::npos) || (texStr.find("resources/obj") != std::string::npos);
            if (!texPath.is_absolute() && !containsResourcesObj) {
                texPath = fs::path("resources") / "obj" / texPath;
            }

            // デバッグ出力（解決結果を確認）
            {
                std::string msg = "[ObjClass] Resolved texture path: " + texPath.string() + "\n";
                OutputDebugStringA(msg.c_str());
            }

            tex->Initialize(texPath.string());
            res->textureHandle_ = tex->GetTextureSrvHandleGPU();
        } else if (!res->textureHandle_.ptr) {
            res->materialData_->hasTexture = false;
            res->textureHandle_ = textureManager_->GetWhiteTextureHandle();
        }

        // ライト
        res->directionalLightResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(DirectionalLight));
        res->directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->directionalLightData_));
        res->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
        res->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
        res->directionalLightData_->intensity = 1.0f;

        // カメラ
        res->cameraResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(CameraForGPU));
        res->cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->cameraData_));
        res->cameraData_->worldPosition = camera_->GetTranslate();

        textures_.push_back(std::move(tex));
        resources_.push_back(std::move(res));
    }

}

void ObjClass::Update(const char* objName) {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());

    for (size_t i = 0; i < resources_.size(); ++i) {
        auto& res = resources_[i];
        std::string meshLabel = "Mesh[" + std::to_string(i) + "]";
        if (ImGui::TreeNode(meshLabel.c_str())) {
            ui_->DebugTransform(res->transform_);
            ui_->DebugMaterialBy3D(res->materialData_);
            ui_->DebugDirectionalLight(res->directionalLightData_);
            ui_->DebugUvTransform(res->uvTransform_);
            ImGui::TreePop();
        }
    }
    ImGui::End();
#endif

    for (auto& res : resources_) {
        res->transformationMatrix_.world = Math::MakeAffineMatrix(res->transform_.scale, res->transform_.rotate, res->transform_.translate);
        res->transformationMatrix_.WVP = Math::Multiply(res->transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
        // 法線変換用：平行移動を除いた World を使う
        Matrix4x4 worldForNormal = res->transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f;
        worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f;
        worldForNormal.m[3][3] = 1.0f;

        // 逆転置行列を計算
        res->transformationMatrix_.WorldInverseTranspose =
            Math::Transpose(Math::Inverse(worldForNormal));

        // 定数バッファへ全フィールドを書き込む
        *res->transformationData_ = {
            res->transformationMatrix_.WVP,
            res->transformationMatrix_.world,
            res->transformationMatrix_.WorldInverseTranspose
        };
        res->materialData_->uvTransform = Math::MakeAffineMatrix(res->uvTransform_.scale, res->uvTransform_.rotate, res->uvTransform_.translate);
        res->directionalLightData_->direction = Math::Normalize(res->directionalLightData_->direction);
        res->cameraData_->worldPosition = camera_->GetTranslate();
    }
}

void ObjClass::Draw() {
    for (auto& res : resources_) {
        drawManager_->DrawByVertex(res.get());
    }
}
