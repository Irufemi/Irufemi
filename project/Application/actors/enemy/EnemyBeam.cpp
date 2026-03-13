#include "EnemyBeam.h"
#include "core/math/geometry/Math.h"
#include <cmath>

void EnemyBeam::Initialize(Camera* camera, const Matrix4x4& muzzleMatrix) {
    obj_ = std::make_unique<ObjClass>();
    // 立方体モデルをビームとして使用
    obj_->Initialize(camera, "sample/block.obj");
    obj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });

    // 初期座標を銃口に合わせる
    transform_.translate = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };

    // 向きを銃口の向きに合わせる
    Vector3 forward = { muzzleMatrix.m[2][0], muzzleMatrix.m[2][1], muzzleMatrix.m[2][2] };
    transform_.rotate.y = std::atan2(forward.x, forward.z);

    // 基本サイズ設定
    transform_.scale = { beamThickness_, beamThickness_, beamLength_ };

    // ビームの端が発射口に来るように中心を前方にずらす
    forwardOffset_ = beamLength_ * 0.5f;

    timer_ = 0.0f;
    isExpired_ = false;
    isActive_ = false;
}

void EnemyBeam::Update(const Vector3& headPos, const Vector3& playerPos) {
    // 1. 向きと距離の計算
    Vector3 diff = Math::Subtract(playerPos, headPos);
    float distance = Math::Length(diff);
    Vector3 direction = Math::Normalize(diff);

    // 2. スケールの設定
    // Zスケールを「距離」そのものにする
    transform_.scale = { beamThickness_, beamThickness_, distance };

    // 3. 座標の設定（ここがあなたのアイデア！）
    // 始点と終点の「ちょうど真ん中」をビーム（モデル中心）の位置にする
    transform_.translate = {
        (headPos.x + playerPos.x) * 0.5f,
        (headPos.y + playerPos.y) * 0.5f,
        (headPos.z + playerPos.z) * 0.5f
    };

    // 4. 回転の設定
    // 常にプレイヤーの方向を向くように設定
    transform_.rotate.y = std::atan2(direction.x, direction.z);
    float distXZ = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    transform_.rotate.x = std::atan2(-direction.y, distXZ);

    obj_->SetTransform(transform_);
    obj_->Update();
}

void EnemyBeam::Draw() {
    if (obj_ && isActive_) {
        obj_->Draw();
    }
}

OBB EnemyBeam::GetOBB() const {
    OBB obb;
    // visualTransform相当の計算
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(transform_.rotate);
    Vector3 currentForward = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    obb.center = Math::Add(transform_.translate, Math::Multiply(forwardOffset_, currentForward));
    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };
    obb.size = transform_.scale;
    return obb;
}