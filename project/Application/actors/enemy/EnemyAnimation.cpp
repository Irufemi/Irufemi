#include "EnemyAnimation.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
}

void EnemyAnimation::Update(Player* player) {
    if (!enemy_) return;
    timer_ += 1.0f / 60.0f; // デルタタイム加算

    EnemyState state = enemy_->GetState();
    switch (state) {
    case EnemyState::Idle:        UpdateIdle(); break;
    case EnemyState::Attack_Beam: UpdateAttackBeam(player); break;
    }
}

void EnemyAnimation::UpdateIdle() {
    attackTimer_ = 0.0f;
    isLockedOn_ = false;
    isFiring_ = false;

    // --- 攻撃フェーズからの復帰処理 ---
    // ズレた各パーツのオフセットを (0,0,0) へ戻していく
    float s = returnSpeed_;
    enemy_->GetHeadMidOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadMidOffset()) * s;
    enemy_->GetHeadLeftOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadLeftOffset()) * s;
    enemy_->GetHeadRightOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadRightOffset()) * s;

    // 胴体パーツの復帰とふわふわアニメ
    for (int i = 0; i < 3; ++i) {
        Vector3& offset = enemy_->GetBodyOffset(i);
        float waveY = std::sin(timer_ * idleWaveSpeed_ + (float)i * idlePhaseOffset_) * idleWaveHeight_;

        offset.x += (0.0f - offset.x) * s;
        offset.y += (waveY - offset.y) * lerpSpeed_; // Y軸のみサイン波を優先
        offset.z += (0.0f - offset.z) * s;
    }

    enemy_->GetGlobalTransform().rotate.y += idleRotationSpeed_;
}

void EnemyAnimation::UpdateAttackBeam(Player* player) {
    attackTimer_ += 1.0f / 60.0f;
    EnemyBeam* beam = enemy_->GetBeam();

    // 中央の頭（発射口）のワールド座標を取得
    Matrix4x4 headMatrix = enemy_->GetHeadMidWorldMatrix();
    Vector3 headPos = { headMatrix.m[3][0], headMatrix.m[3][1] + headExtensionY_, headMatrix.m[3][2] };

    // --- フェーズ1：チャージ（追尾中） ---
    if (attackTimer_ < chargeTime_) {
        isFiring_ = false;
        Vector3 currentTarget = (player) ? player->GetTranslate() : Vector3{ 0,0,0 };
        currentTarget.y += 1.0f;

        // プレイヤーの方向を向く回転計算
        Vector3 ePos = enemy_->GetGlobalTransform().translate;
        float tAngleY = std::atan2(currentTarget.x - ePos.x, currentTarget.z - ePos.z);
        float diffY = NormalizeAngle(tAngleY - enemy_->GetGlobalTransform().rotate.y);
        enemy_->GetGlobalTransform().rotate.y += diffY * beamRotateSpeed_;

        enemy_->FireBeam();
        if (beam) {
            beam->SetActive(true);
            beam->SetThickness(beamThicknessCharge_);
            beam->SetColor({ 1.0f, 0.0f, 0.0f, 0.3f });
            beam->Update(headPos, currentTarget);
        }
    }
    // --- フェーズ2：本射（シェイク・固定射撃） ---
    else if (attackTimer_ < (chargeTime_ + fireTime_)) {
        isFiring_ = true;
        if (!isLockedOn_) {
            if (player) { lockedTargetPos_ = player->GetTranslate(); lockedTargetPos_.y += 1.0f; }
            isLockedOn_ = true;
        }

        float sp = shakeBaseSpeed_;
        float hStr = headShakeStrength_;
        float bStr = bodyShakeStrength_;

        // --- 頭部の激しいシェイク ---
        enemy_->GetHeadMidOffset() = { std::sin(timer_ * sp) * hStr,      std::cos(timer_ * sp * 1.1f) * hStr, 0 };
        enemy_->GetHeadLeftOffset() = { std::sin(timer_ * sp * 0.9f) * hStr, std::cos(timer_ * sp * 1.2f) * hStr, 0 };
        enemy_->GetHeadRightOffset() = { std::sin(timer_ * sp * 1.3f) * hStr, std::cos(timer_ * sp * 0.8f) * hStr, 0 };

        // --- ★胴体パーツのシェイク（頭部より控えめに震わせる） ---
        for (int i = 0; i < 3; ++i) {
            Vector3& offset = enemy_->GetBodyOffset(i);
            // パーツごとに少しずつ周期(i*0.5f)をずらしてバラバラ感を出す
            offset.x = std::sin(timer_ * sp + (float)i * 0.5f) * bStr;
            offset.z = std::cos(timer_ * sp * 0.9f + (float)i * 0.5f) * bStr;
        }

        if (beam) {
            beam->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
            beam->SetThickness(beamThicknessFire_);
            beam->Update(headPos, lockedTargetPos_);
        }
    }
    // --- フェーズ3：後隙（個別パーツの縮退） ---
    else if (attackTimer_ < (chargeTime_ + fireTime_ + recoveryTime_)) {
        isFiring_ = false;
        if (beam) beam->SetActive(false);

        float sSpd = shrinkSpeed_;
        // 胴体：配列に定義した個別の目標座標へ寄せる
        for (int i = 0; i < 3; ++i) {
            Vector3& offset = enemy_->GetBodyOffset(i);
            offset += (shrinkBodyTargets_[i] - offset) * sSpd;
        }
        // 頭部：それぞれ別の目標座標へ寄せる
        enemy_->GetHeadMidOffset() += (shrinkHeadMidTarget_ - enemy_->GetHeadMidOffset()) * sSpd;
        enemy_->GetHeadLeftOffset() += (shrinkHeadLeftTarget_ - enemy_->GetHeadLeftOffset()) * sSpd;
        enemy_->GetHeadRightOffset() += (shrinkHeadRightTarget_ - enemy_->GetHeadRightOffset()) * sSpd;

        enemy_->GetGlobalTransform().rotate.x *= (1.0f - sSpd); // 傾きを戻す
    } else {
        enemy_->SetState(EnemyState::Idle);
    }
}

float EnemyAnimation::NormalizeAngle(float angle) {
    while (angle > M_PI)  angle -= 2.0f * (float)M_PI;
    while (angle < -M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}