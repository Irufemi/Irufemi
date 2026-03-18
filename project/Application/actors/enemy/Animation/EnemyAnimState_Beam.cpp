#include "EnemyAnimState_Beam.h"
#include "Enemy.h"
#include "actors/player/Player.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

void EnemyAnimState_Beam::Enter(Enemy* enemy) {
    // 攻撃開始時にタイマーとフラグをリセット
    attackTimer_ = 0.0f;
    isLockedOn_ = false;
    isFiring_ = false;
    hasFinishedAttack_ = false;
}

void EnemyAnimState_Beam::Update(Enemy* enemy, Player* player, float deltaTime) {
    attackTimer_ += deltaTime;

    // 各フェーズの終了時間を計算
    float endCharge = chargeTime_;
    float endAnticipation = endCharge + anticipationTime_;
    float endFire = endAnticipation + fireTime_;
    float endStun = endFire + stunTime_;
    float endRecovery = endStun + recoveryTime_;

    // --- 元の UpdateAttackBeam() のロジックを完全移植 ---

    // 1. チャージフェーズ：追尾と首の集結
    if (attackTimer_ < endCharge) {
        Vector3 playerPos = player->GetTranslate();
        Vector3 enemyPos = enemy->GetGlobalTransform().translate;
        Vector3 toPlayer = { playerPos.x - enemyPos.x, playerPos.y - enemyPos.y, playerPos.z - enemyPos.z };

        float targetRotY = std::atan2(toPlayer.x, toPlayer.z);
        float distXZ = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.z * toPlayer.z);
        float targetRotX = std::atan2(-toPlayer.y, distXZ);

        // 追尾（回転補間）
        enemy->GetGlobalTransform().rotate.y += NormalizeAngle(targetRotY - enemy->GetGlobalTransform().rotate.y) * 0.05f;
        enemy->GetGlobalTransform().rotate.x += NormalizeAngle(targetRotX - enemy->GetGlobalTransform().rotate.x) * 0.05f;

        // 首を中央に寄せる
        auto GatherHead = [&](Vector3& offset, float targetX) {
            offset.x += (targetX - offset.x) * 0.05f;
            offset.y += (0.0f - offset.y) * 0.05f;
            offset.z += (gatherStrength_ - offset.z) * 0.05f;
            };
        GatherHead(enemy->GetHeadMidOffset(), 0.0f);
        GatherHead(enemy->GetHeadLeftOffset(), -1.0f);
        GatherHead(enemy->GetHeadRightOffset(), 1.0f);
    }
    // 2. 溜め（予備動作）：ロックオン固定と小刻みな震え
    else if (attackTimer_ < endAnticipation) {
        if (!isLockedOn_) {
            isLockedOn_ = true;
            lockedTargetPos_ = player->GetTranslate();
        }
        float shake = std::sin(attackTimer_ * 50.0f) * shakeStrength_;
        enemy->GetHeadMidOffset().x += shake;
    }
    // 3. 発射フェーズ
    else if (attackTimer_ < endFire) {
        if (!isFiring_) {
            isFiring_ = true;
            enemy->FireBeam(); // 弾丸/ビーム生成
        }
    }
    // 4. 硬直（反動）：撃った後の反動演出
    else if (attackTimer_ < endStun) {
        isFiring_ = false;
        float sp = 40.0f;
        auto SetStunPos = [&](Vector3& offset, float seed) {
            offset.x = std::sin(attackTimer_ * sp * seed) * stunShakeStrength_;
            offset.z += (-1.0f - offset.z) * 0.15f;
            };
        SetStunPos(enemy->GetHeadMidOffset(), 1.0f);
        SetStunPos(enemy->GetHeadLeftOffset(), 1.1f);
        SetStunPos(enemy->GetHeadRightOffset(), 0.9f);
    }
    // 5. 回復：ガクッと力を抜いて元の姿勢へ戻る
    else if (attackTimer_ < endRecovery) {
        float recProgress = (attackTimer_ - endStun) / recoveryTime_;
        float breathCurve = std::sin(recProgress * (float)M_PI);
        float currentExhaustion = (1.0f - recProgress) * exhaustionDepth_ - (breathCurve * 0.5f);

        auto ApplyExhaustion = [&](Vector3& offset) {
            offset.y += (currentExhaustion - offset.y) * lerpSpeed_;
            offset.z += (0.0f - offset.z) * returnSpeed_;
            };
        ApplyExhaustion(enemy->GetHeadMidOffset());
        ApplyExhaustion(enemy->GetHeadLeftOffset());
        ApplyExhaustion(enemy->GetHeadRightOffset());
    } else {
        // 全フェーズ完了
        hasFinishedAttack_ = true;
    }
}

void EnemyAnimState_Beam::Exit(Enemy* enemy) {
    isFiring_ = false;
}

float EnemyAnimState_Beam::NormalizeAngle(float angle) {
    while (angle > (float)M_PI) angle -= 2.0f * (float)M_PI;
    while (angle < -(float)M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}