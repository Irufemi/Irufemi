#include "EnemyAnimation.h"
#include "Enemy.h"
#include <cmath>

void EnemyAnimation::Initialize(Enemy* enemy) {
    enemy_ = enemy;
}

void EnemyAnimation::Update() {
    if (!enemy_) return;
    timer_ += 1.0f / 60.0f;

    EnemyState state = enemy_->GetState();
    const float lerpSpeed = 0.1f;

    if (state == EnemyState::Idle) {
        // 攻撃タイマーをリセットしておく
        attackTimer_ = 0.0f;

        // --- 待機アニメーション（ふわふわ） ---
        for (int i = 0; i < 3; ++i) {
            float targetY = std::sin(timer_ * 2.0f + (float)i * 0.8f) * 0.15f;
            Vector3& offset = enemy_->GetBodyOffset(i);
            offset.y += (targetY - offset.y) * lerpSpeed;
            offset.x += (0.0f - offset.x) * lerpSpeed;
        }

        float headTargetY = std::sin(timer_ * 2.0f + 2.4f) * 0.2f;
        auto& offL = enemy_->GetHeadLeftOffset();
        offL.y += (headTargetY - offL.y) * lerpSpeed;
        offL.x += (0.0f - offL.x) * lerpSpeed;

        auto& offM = enemy_->GetHeadMidOffset();
        offM.y += (headTargetY - offM.y) * lerpSpeed;
        offM.x += (0.0f - offM.x) * lerpSpeed;

        auto& offR = enemy_->GetHeadRightOffset();
        offR.y += (headTargetY - offR.y) * lerpSpeed;
        offR.x += (0.0f - offR.x) * lerpSpeed;

    } else if (state == EnemyState::Attack) {
        attackTimer_ += 1.0f / 60.0f;

        // 1.0秒間「中央に寄る」フェーズ、その後「シェイク」フェーズ
        float gatheringTime = 1.0f;
        float shakingTime = 1.5f;

        if (attackTimer_ < gatheringTime) {
            // --- フェーズ1: 中央にギュッと寄る ---
            float gatherLerp = 0.15f; // 少し早めに寄せる

            // 左右の頭が中央(X=0)へ
            enemy_->GetHeadLeftOffset().x += (1.0f - enemy_->GetHeadLeftOffset().x) * gatherLerp;
            enemy_->GetHeadRightOffset().x += (-1.0f - enemy_->GetHeadRightOffset().x) * gatherLerp;

            // 全体的に少し沈ませる
            float sinkY = -0.4f;
            enemy_->GetHeadMidOffset().y += (sinkY - enemy_->GetHeadMidOffset().y) * gatherLerp;
            for (int i = 0; i < 3; ++i) {
                enemy_->GetBodyOffset(i).y += (sinkY * 0.5f - enemy_->GetBodyOffset(i).y) * gatherLerp;
            }

        } else if (attackTimer_ < (gatheringTime + shakingTime)) {
            // --- フェーズ2: 全身で激しくシェイク ---
            float shakeX = std::sin(timer_ * 80.0f) * 0.15f;
            float shakeY = std::cos(timer_ * 90.0f) * 0.05f;

            // 頭部3つを激しくシェイク
            enemy_->GetHeadMidOffset().x = shakeX;
            enemy_->GetHeadMidOffset().y = -0.4f + shakeY;
            enemy_->GetHeadLeftOffset().x = 1.0f + shakeX;
            enemy_->GetHeadRightOffset().x = -1.0f + shakeX;

            // 胴体も頭に合わせてシェイク
            for (int i = 0; i < 3; ++i) {
                enemy_->GetBodyOffset(i).x = shakeX * 0.6f;
                enemy_->GetBodyOffset(i).y = -0.2f + shakeY;
            }

        } else {
            // --- 攻撃終了: 待機状態へ戻す ---
            enemy_->SetState(EnemyState::Idle);
        }
    }
}