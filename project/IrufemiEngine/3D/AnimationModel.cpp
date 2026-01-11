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

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    // ModelManagerから共有モデルを取得するだけ
    managedModel_ = modelManager_->GetModel(filename);

    if (!managedModel_ || !managedModel_->cpuModel) {
        OutputDebugStringA("[ObjClass] Initialize: model load failed.\n");
        return;
    }

    // 変換行列リソースの生成とマップ
    assert(drawManager_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationResource_ = drawManager_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix));
    transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));


    // --- インスタンス固有リソースの生成 ---
    // マテリアル
    materialResource_ = GetDirectXCommon()->CreateBufferResource(sizeof(Material));
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

    rootNodeAnimation = animation_.nodeAnimations[managedModel_->cpuModel->rootNode.name]; // rootNodeのAnimationを取得
    Vector3 translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime); //指定時刻の値を取得。関数の詳細は次ページ
    Quaternion rotation = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime);
    transform_.rotate = Math::ToEuler(rotation);
    transform_.scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime);
    localMatrix_ = Math::MakeAffineMatrix(transform_.scale, rotation, transform_.translate);

    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, rotation, transform_.translate);
    Matrix4x4 worldViewProj = Math::Multiply(transformationMatrix_.world, Math::Multiply(camera_->GetViewMatrix(), camera_->GetPerspectiveFovMatrix()));

    // rootNode 行列を適用
    if (managedModel_->cpuModel) {
        worldViewProj = managedModel_->cpuModel->rootNode.localMatrix * worldViewProj;
        transformationMatrix_.world = managedModel_->cpuModel->rootNode.localMatrix * transformationMatrix_.world;
    }

    transformationMatrix_.WVP = worldViewProj;

    Matrix4x4 worldForNormal = r->transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
    r->transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
    r->transformationData_->WorldInverseTranspose = r->transformationMatrix_.WorldInverseTranspose;

    r->materialData_->uvTransform = Math::MakeAffineMatrix(r->uvTransform_.scale, r->uvTransform_.rotate, r->uvTransform_.translate);

    // 出来上がったlocalMatrixでモデルをアニメーションさせ、worldMatrixで任意の変換をかける
    r->transformationData_->WVP = localMatrix_ * worldMatrix_ * (camera_->GetViewMatrix() * camera_->GetPerspectiveFovMatrix());
    r->transformationData_->world = localMatrix_ * worldMatrix_;

}

// 描画
void AnimationModel::Draw() {

}

// デバッグ
void AnimationModel::Debug() {

}