#include "AnimationModel.h"

#include "Application/camera/Camera.h"
#include "manager/ModelManager.h"
#include "manager/AnimationManager.h"
#include "engine/directX/DirectXCommon.h"
#include "function/Math.h"
#include <cmath>

// 初期化
void AnimationModel::Initialize(Camera* camera, const std::string& directoryPath, const std::string& filename) {

    camera_ = camera;
    textures_.clear();
    instanceResources_.clear();

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    managedModel_ = modelManager_->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[ObjClass] Initialize: model load failed.\n");
        return;
    }

    const auto& cpuModel = managedModel_->cpuModel;
    textures_.reserve(cpuModel->meshes.size());
    instanceResources_.reserve(cpuModel->meshes.size());

    for (const auto& mesh : cpuModel->meshes) {
        auto res = std::make_unique<D3D12ResourceUtil>();

        // --- インスタンス固有リソースの生成 ---
        // マテリアル
        res->materialResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(Material));
        res->materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->materialData_));

        // ObjMaterial から Material へ必要なデータをコピー
        res->materialData_->color = mesh.material.color;
        res->materialData_->enableLighting = mesh.material.enableLighting;
        res->materialData_->uvTransform = mesh.material.uvTransform;
        res->materialData_->shininess = mesh.material.shininess;
        res->materialData_->hasTexture = !mesh.material.textureFilePath.empty();
        res->materialData_->lightingMode = mesh.material.enableLighting ? 2 : 0;

        if (res->materialData_->color.w <= 0.0f) { res->materialData_->color.w = 1.0f; }

        // 行列
        res->transformationResource_ = res->GetDirectXCommon()->CreateBufferResource(sizeof(TransformationMatrix));
        res->transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&res->transformationData_));

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

        // テクスチャ（これはインスタンスごとではなく、本来はMaterial共有が望ましいが、今回は従来通り）
        auto tex = std::make_unique<Texture>();
        if (!mesh.material.textureFilePath.empty()) {
            // (パス解決処理は変更なし)
            // ...
            tex->Initialize(mesh.material.textureFilePath);
            res->textureHandle_ = tex->GetTextureSrvHandleGPU();
            res->materialData_->hasTexture = true;
        } else {
            res->materialData_->hasTexture = false;
            res->textureHandle_ = textureManager_->GetWhiteTextureHandle();
        }

        textures_.push_back(std::move(tex));
        instanceResources_.push_back(std::move(res));
    }

    animation_ = AnimationManager::LoadAnimationFile(directoryPath, filename);

    /// Animationを再生する
    animationTime = 0.0f;
}

// 更新
void AnimationModel::Update() {

    /// Animationを再生する

    // 時刻を進めて、指定した時刻の各種データを取得し、localMatrixを生成する

    animationTime += 1.0f / 60.0f; //時刻を進める。1/60で固定してあるが、計測した時間を使って可変フレーム対応するほうが望ましい
    animationTime = std::fmod(animationTime, animation_.duration); // 最後までいったら最初からリピート再生。リピートしなくても別に良い
    rootNodeAnimation = animation_.nodeAnimations[model_.rootNode.name]; // rootNodeのAnimationを取得
    transform_.translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime); //指定時刻の値を取得。関数の詳細は次ページ
    transform_.rotate = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime);
    transform_.scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime);
    localMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    // 出来上がったlocalMatrixでモデルをアニメーションさせ、worldMatrixで任意の変換をかける
    transformData_->WVP = localMatrix_ * worldMatrix_ * (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
    transformData_->world = localMatrix_ * worldMatrix_;

}

// 描画
void AnimationModel::Draw() {

}

// デバッグ
void AnimationModel::Debug() {

}