#include "AnimationModel.h"

#include "Application/camera/Camera.h"
#include "manager/ModelManager.h"
#include "manager/AnimationManager.h"
#include "function/Math.h"
#include <cmath>

// 初期化
void AnimationModel::Initialize(Camera* camera, const std::string& directoryPath, const std::string& filename) {

    model_ = ModelManager::LoadModelFile(directoryPath, filename);
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
    NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[model_.rootNode.name]; // rootNodeのAnimationを取得
    transform_.translate = CalculateValue(rootNodeAnimation.translate, animationTime); //指定時刻の値を取得。関数の詳細は次ページ
    transform_.rotate CalculateValue(rootNodeAnimation.rotate, animationTime);
    transform_.scale = CalculateValue(rootNodeAnimation.scale, animationTime);
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