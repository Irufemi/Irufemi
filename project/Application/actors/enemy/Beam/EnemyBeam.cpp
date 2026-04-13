#include "EnemyBeam.h"
#include "Core/Math/Math.h"
#include <cmath>

void EnemyBeam::Initialize(Camera* camera, const Matrix4x4& muzzleMatrix) {
    telegraphObj_ = std::make_unique<ObjClass>();
    telegraphObj_->Initialize(camera, "sample/block.obj");
    telegraphObj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });

    attackObj_ = std::make_unique<ObjClass>();
    attackObj_->Initialize(camera, "sample/block.obj");
    attackObj_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f });

    Vector3 initialPos = { muzzleMatrix.m[3][0], muzzleMatrix.m[3][1], muzzleMatrix.m[3][2] };
    Vector3 forward = { muzzleMatrix.m[2][0], muzzleMatrix.m[2][1], muzzleMatrix.m[2][2] };
    float initialRotateY = std::atan2(forward.x, forward.z);

    telegraphTransform_.translate = initialPos;
    telegraphTransform_.rotate = { 0.0f, initialRotateY, 0.0f };
    telegraphTransform_.scale = { telegraphThickness_, telegraphThickness_, beamLength_ };
    telegraphForwardOffset_ = beamLength_ * 0.5f;

    attackTransform_.translate = initialPos;
    attackTransform_.rotate = { 0.0f, initialRotateY, 0.0f };
    attackTransform_.scale = { attackThickness_, attackThickness_, beamLength_ };
    attackForwardOffset_ = beamLength_ * 0.5f;

    isExpired_ = false;
    isTelegraphActive_ = false;
    isAttackActive_ = false;
}

void EnemyBeam::Update(const Vector3& headPos, const Vector3& playerPos) {
    Vector3 diff = Math::Subtract(playerPos, headPos);
    float distance = Math::Length(diff);
    Vector3 direction = Math::Normalize(diff);

    Vector3 center = {
        (headPos.x + playerPos.x) * 0.5f,
        (headPos.y + playerPos.y) * 0.5f,
        (headPos.z + playerPos.z) * 0.5f
    };

    Vector3 rotate = { 0.0f, 0.0f, 0.0f };
    rotate.y = std::atan2(direction.x, direction.z);
    float distXZ = std::sqrt(direction.x * direction.x + direction.z * direction.z);
    rotate.x = std::atan2(-direction.y, distXZ);

    if (isTelegraphActive_) {
        telegraphTransform_.scale = { telegraphThickness_, telegraphThickness_, distance };
        telegraphTransform_.translate = center;
        telegraphTransform_.rotate = rotate;
        telegraphObj_->SetTransform(telegraphTransform_);
        telegraphObj_->Update();
    }

    if (isAttackActive_) {
        attackTransform_.scale = { attackThickness_, attackThickness_, distance };
        attackTransform_.translate = center;
        attackTransform_.rotate = rotate;
        attackObj_->SetTransform(attackTransform_);
        attackObj_->Update();
    }
}

void EnemyBeam::Draw(IrufemiEngine* engine) {
    if (!engine) return;

    engine->ApplyPSO();
    if (isTelegraphActive_ && telegraphObj_) {
        telegraphObj_->Draw();
    }
    if (isAttackActive_ && attackObj_) {
        attackObj_->Draw();
    }
}

OBB EnemyBeam::GetOBB() const {
    OBB obb;
    // 攻撃ビームのトランスフォームを基準にする
    obb.center = attackTransform_.translate;
    Matrix4x4 rotateMat = Math::MakeRotateXYZMatrix(attackTransform_.rotate);
    Vector3 currentForward = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    // moto版と同じオフセット調整を適用
    obb.center = Math::Add(attackTransform_.translate, Math::Multiply(attackForwardOffset_, currentForward));

    obb.orientations[0] = { rotateMat.m[0][0], rotateMat.m[0][1], rotateMat.m[0][2] };
    obb.orientations[1] = { rotateMat.m[1][0], rotateMat.m[1][1], rotateMat.m[1][2] };
    obb.orientations[2] = { rotateMat.m[2][0], rotateMat.m[2][1], rotateMat.m[2][2] };

    obb.size = { attackTransform_.scale.x * 0.5f, attackTransform_.scale.y * 0.5f, attackTransform_.scale.z * 0.5f };
    return obb;
}