#include "EnemyBeam.h"
#include <cmath>

void EnemyBeam::Initialize(Camera* camera, const Matrix4x4& muzzleMatrix) {
    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(camera, "sample/block.obj");
    obj_->SetColor({1.0f, 1.0f, 0.0f,0.5f});

    // 1. 基本となる座標と回転を設定
    transform_.translate = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };

    Vector3 forward = { muzzleMatrix.m[2][0], muzzleMatrix.m[2][1], muzzleMatrix.m[2][2] };
    transform_.rotate.y = std::atan2(forward.x, forward.z);
    transform_.rotate.x = 0.785f; // 下斜め45度

    // 2. ビームの長さを設定
    transform_.scale = { beamThickness_, beamThickness_, beamLength_ };

    // 3. 重要：ビームの中心を「前方に半分」ずらす
    // これにより、ビームの端がちょうど発射口（muzzleMatrixの位置）に来るようになります
    forwardOffset_ = beamLength_ * 0.5f;
}

void EnemyBeam::Update(const Matrix4x4& muzzleMatrix) {
    timer_ += 1.0f / 60.0f;

    // 発射口の位置を更新
    transform_.translate = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };

    // 描画用のモデルも、中心をずらして更新する
    // 本来の translate に forward方向 * offset 分を足した位置をセット
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    Vector3 currentForward = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    Transform visualTransform = transform_;
    visualTransform.translate.x += currentForward.x * forwardOffset_;
    visualTransform.translate.y += currentForward.y * forwardOffset_ * 0.5f;
    visualTransform.translate.z += currentForward.z * forwardOffset_;

    if (timer_ > lifeTime_) isExpired_ = true;

    obj_->SetTransform(visualTransform);
    obj_->Update();
}

void EnemyBeam::Draw() {
    // 将来的にパーティクルのみにする際は、ここをコメントアウトするか isVisible_ で制御
    if (obj_ && isVisible_) obj_->Draw();
}

OBB EnemyBeam::GetOBB() const {
    OBB obb;
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    
    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    // 判定の中心点を「前方」にずらす
    Vector3 forward = obb.orientations[2];
    obb.center = {
        transform_.translate.x + forward.x * forwardOffset_,
        transform_.translate.y + forward.y * forwardOffset_ * 0.5f,
        transform_.translate.z + forward.z * forwardOffset_
    };

    obb.size = { transform_.scale.x * 0.5f, transform_.scale.y * 0.5f, beamLength_ * 0.5f };

    return obb;
}
