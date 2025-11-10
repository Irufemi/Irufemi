#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>
#include "source/Texture.h"
#include "function/Function.h"
#include "function/Math.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "manager/DebugUI.h"
#include "manager/ModelManager.h"
#include <imgui.h>
#include "engine/directX/DirectXCommon.h"

TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;
ModelManager* ObjClass::modelManager_ = nullptr;

void ObjClass::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;
    textures_.clear();
    resources_.clear();

    // モデル取得（キャッシュ経由）
    if (modelManager_) {
        objModel_ = modelManager_->GetModel(filename);
    } else {
        // フォールバック（旧挙動）
        objModel_ = std::make_shared<ObjModel>(ModelManager::LoadModelFileM("resources/obj", filename));
    }

    if (!objModel_) {
        OutputDebugStringA("[ObjClass] Initialize: model load failed.\n");
        return;
    }

    textures_.reserve(objModel_->meshes.size());
    resources_.reserve(objModel_->meshes.size());

    for (const auto& mesh : objModel_->meshes) {
        auto res = std::make_unique<D3D12ResourceUtil>();

        // 頂点バッファ
        res->vertexResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(VertexData) * mesh.vertices.size());
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
        res->materialData_->uvTransform = mesh.material.uvTransform;
        res->materialData_->shininess = 64.0f;

        if (res->materialData_->color.w <= 0.0f) { res->materialData_->color.w = 1.0f; }

        // 行列初期
        res->transformationMatrix_.world =
            Math::MakeAffineMatrix(res->transform_.scale, res->transform_.rotate, res->transform_.translate);
        res->transformationMatrix_.WVP =
            Math::Multiply(res->transformationMatrix_.world,
                           Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));
        res->transformationResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(TransformationMatrix));
        res->transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->transformationData_));

        // 法線変換用：平行移動を除いた World を使う
        Matrix4x4 worldForNormal = res->transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;

        // 逆転置行列を計算
        res->transformationMatrix_.WorldInverseTranspose =
            Math::Transpose(Math::Inverse(worldForNormal));

        // 定数バッファへ全フィールドを書き込む
        *res->transformationData_ = {
            res->transformationMatrix_.WVP,
            res->transformationMatrix_.world,
            res->transformationMatrix_.WorldInverseTranspose
        };

        /*glTFを読み込んでみよう*/

        // 共有 rootNode の行列適用
        res->transformationData_->WVP = objModel_->rootNode.localMatrix *
            res->transformationMatrix_.world *
            (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
        res->transformationData_->world = objModel_->rootNode.localMatrix * res->transformationMatrix_.world;

        // 上記のコードでは、描画時だけ適用するようになっている
        // ゲーム中にも利用したい場合、この値をうまく扱えるようにしていく必要があるが、まずは描画時だけ適用しておいて、慣れてから対応法を考えると良い

        // テクスチャ
        auto tex = std::make_unique<Texture>();
        if (!mesh.material.textureFilePath.empty()) {
            std::string texStr = mesh.material.textureFilePath;
            std::replace(texStr.begin(), texStr.end(), '/', '\\');
            namespace fs = std::filesystem;
            fs::path texPath(texStr);
            bool containsRoot = (texStr.find("resources\\obj") != std::string::npos) ||
                                (texStr.find("resources/obj") != std::string::npos);
            if (!texPath.is_absolute() && !containsRoot) {
                texPath = fs::path("resources") / "obj" / texPath;
            }
            tex->Initialize(texPath.string());
            res->textureHandle_ = tex->GetTextureSrvHandleGPU();
            res->materialData_->hasTexture = true;
        } else {
            res->materialData_->hasTexture = false;
            res->textureHandle_ = textureManager_->GetWhiteTextureHandle();
        }

        // ライト
        res->directionalLightResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(DirectionalLight));
        res->directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->directionalLightData_));
        res->directionalLightData_->color = { 1,1,1,1 };
        res->directionalLightData_->direction = { 0,-1,0 };
        res->directionalLightData_->intensity = 1.0f;

        // カメラ
        res->cameraResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(CameraForGPU));
        res->cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->cameraData_));
        res->cameraData_->worldPosition = camera_->GetTranslate();

#if defined(_DEBUG) || defined(DEVELOPMENT)
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "[ObjClass] mesh vtx=%zu hasTex=%d srv=0x%llX (cached)\n",
            res->vertexDataList_.size(),
            res->materialData_->hasTexture,
            static_cast<unsigned long long>(res->textureHandle_.ptr));
        OutputDebugStringA(buf);
#endif

        textures_.push_back(std::move(tex));
        resources_.push_back(std::move(res));
    }
}

void ObjClass::Update(const char* objName) {
#if defined(_DEBUG) || defined(DEVELOPMENT)
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    for (size_t i = 0; i < resources_.size(); ++i) {
        auto& r = resources_[i];
        std::string meshLabel = "Mesh[" + std::to_string(i) + "]";
        if (ImGui::TreeNode(meshLabel.c_str())) {
            ui_->DebugTransform(r->transform_);
            ui_->DebugMaterialBy3D(r->materialData_);
            ui_->DebugDirectionalLight(r->directionalLightData_);
            ui_->DebugUvTransform(r->uvTransform_);
            ImGui::TreePop();
        }
    }
    ImGui::End();
#endif

    for (auto& r : resources_) {
        r->transformationMatrix_.world =
            Math::MakeAffineMatrix(r->transform_.scale, r->transform_.rotate, r->transform_.translate);

        r->transformationMatrix_.WVP =
            Math::Multiply(r->transformationMatrix_.world,
                           Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

        Matrix4x4 worldForNormal = r->transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        // 逆転置行列を計算
        r->transformationMatrix_.WorldInverseTranspose =
            Math::Transpose(Math::Inverse(worldForNormal));
        
        // 定数バッファへ全フィールドを書き込む
        *r->transformationData_ = {
            r->transformationMatrix_.WVP,
            r->transformationMatrix_.world,
            r->transformationMatrix_.WorldInverseTranspose
        };


        /*glTFを読み込んでみよう*/

        /// Matrixを適用する

        // 上記のコードでは、描画時だけ適用するようになっている
        // ゲーム中にも利用したい場合、この値をうまく扱えるようにしていく必要があるが、まずは描画時だけ適用しておいて、慣れてから対応法を考えると良い

        // 共有 rootNode 行列適用
        if (objModel_) {
            r->transformationData_->WVP = objModel_->rootNode.localMatrix *
                r->transformationMatrix_.world *
                (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
            r->transformationData_->world = objModel_->rootNode.localMatrix * r->transformationMatrix_.world;
        }

        r->materialData_->uvTransform =
            Math::MakeAffineMatrix(r->uvTransform_.scale, r->uvTransform_.rotate, r->uvTransform_.translate);
        r->directionalLightData_->direction = Math::Normalize(r->directionalLightData_->direction);
        r->cameraData_->worldPosition = camera_->GetTranslate();
    }
}

void ObjClass::Draw() {
    for (auto& r : resources_) {
        drawManager_->DrawByVertex(r.get());
    }
}
