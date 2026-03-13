#include "EnemyAnimation.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
}

void EnemyAnimation::Update(Player* player) {
    if (!enemy_) return;
    timer_ += 1.0f / 60.0f;

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

    // --- 自然な復帰アニメーション ---
    // 攻撃フェーズで作った「オフセット（ズレ）」を、ゆっくり0に戻していく
    float s = returnSpeed_;
    enemy_->GetHeadMidOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadMidOffset()) * s;
    enemy_->GetHeadLeftOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadLeftOffset()) * s;
    enemy_->GetHeadRightOffset() += (Vector3{ 0,0,0 } - enemy_->GetHeadRightOffset()) * s;

    // 胴体パーツも個別に0に戻しながら、ふわふわさせる
    for (int i = 0; i < 3; ++i) {
        Vector3& offset = enemy_->GetBodyOffset(i);
        float waveY = std::sin(timer_ * idleWaveSpeed_ + (float)i) * idleWaveHeight_;

        offset.x += (0.0f - offset.x) * s;
        offset.y += (waveY - offset.y) * lerpSpeed_; // Yは波に乗せる
        offset.z += (0.0f - offset.z) * s;
    }

    enemy_->GetGlobalTransform().rotate.y += idleRotationSpeed_;
}

void EnemyAnimation::UpdateAttackBeam(Player* player) {
    attackTimer_ += 1.0f / 60.0f;
    EnemyBeam* beam = enemy_->GetBeam();

    // 発射口（中央の頭）の位置を取得
    Matrix4x4 headMatrix = enemy_->GetHeadMidWorldMatrix();
    Vector3 headPos = { headMatrix.m[3][0], headMatrix.m[3][1], headMatrix.m[3][2] };

    // --- フェーズ1：チャージ中 ---
    if (attackTimer_ < chargeTime_) {
        isFiring_ = false;
        Vector3 targetP = (player) ? player->GetTranslate() : Vector3{ 0,0,0 };
        targetP.y += 1.0f;

        // プレイヤーの方向を向く（回転処理）
        Vector3 ePos = enemy_->GetGlobalTransform().translate;
        float tAngleY = std::atan2(targetP.x - ePos.x, targetP.z - ePos.z);
        float diffY = NormalizeAngle(tAngleY - enemy_->GetGlobalTransform().rotate.y);
        enemy_->GetGlobalTransform().rotate.y += diffY * beamRotateSpeed_;

        // 予兆ビームの設定
        enemy_->FireBeam();
        if (beam) {
            beam->SetActive(true);
            beam->SetThickness(beamThicknessCharge_);
            beam->SetColor({ 1.0f, 0.0f, 0.0f, 0.3f });
            beam->Update(headPos, targetP);
        }
    }
    // --- フェーズ2：ビーム本射（激しい振動と固定射撃） ---
    else if (attackTimer_ < (chargeTime_ + fireTime_)) {
        isFiring_ = true;
        if (!isLockedOn_) {
            if (player) { lockedTargetPos_ = player->GetTranslate(); lockedTargetPos_.y += 1.0f; }
            isLockedOn_ = true;
        }

        // 全頭部をバラバラの周期でシェイクさせる
        float sp = shakeBaseSpeed_;
        float st = shakeStrength_;
        enemy_->GetHeadMidOffset() = { std::sin(timer_ * sp) * st,      std::cos(timer_ * sp * 1.1f) * st, 0 };
        enemy_->GetHeadLeftOffset() = { std::sin(timer_ * sp * 0.9f) * st, std::cos(timer_ * sp * 1.2f) * st, 0 };
        enemy_->GetHeadRightOffset() = { std::sin(timer_ * sp * 1.3f) * st, std::cos(timer_ * sp * 0.8f) * st, 0 };

        if (beam) {
            beam->SetThickness(beamThicknessFire_);
            beam->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
            beam->Update(headPos, lockedTargetPos_);
        }
    }
    // --- フェーズ3：撃ち終わりの硬直（ここで各パーツが個別に縮こまる） ---
    else if (attackTimer_ < (chargeTime_ + fireTime_ + recoveryTime_)) {
        isFiring_ = false;
        if (beam) beam->SetActive(false);

        float sSpd = shrinkSpeed_;

        // ★【胴体パーツの個別縮小処理】★
        for (int i = 0; i < 3; ++i) {
            Vector3& offset = enemy_->GetBodyOffset(i);
            // メンバ変数の配列 shrinkBodyTargets_[i] に向かって補完
            offset += (shrinkBodyTargets_[i] - offset) * sSpd;
        }

        // 【各頭部の個別縮小処理】
        enemy_->GetHeadMidOffset() += (shrinkHeadMidTarget_ - enemy_->GetHeadMidOffset()) * sSpd;
        enemy_->GetHeadLeftOffset() += (shrinkHeadLeftTarget_ - enemy_->GetHeadLeftOffset()) * sSpd;
        enemy_->GetHeadRightOffset() += (shrinkHeadRightTarget_ - enemy_->GetHeadRightOffset()) * sSpd;

        // 本体の傾きを戻す
        enemy_->GetGlobalTransform().rotate.x *= 0.9f;
    } else {
        enemy_->SetState(EnemyState::Idle);
    }
}

float EnemyAnimation::NormalizeAngle(float angle) {
    while (angle > M_PI)  angle -= 2.0f * (float)M_PI;
    while (angle < -M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}