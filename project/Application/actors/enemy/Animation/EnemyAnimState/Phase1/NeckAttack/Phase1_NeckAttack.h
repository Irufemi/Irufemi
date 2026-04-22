#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 第1形態 首振り3連撃ステート
 */
class Phase1_NeckAttack : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return hasFinishedAttack_; }
    bool IsFiring() const override { return false; }

private:
    float totalTimer_ = 0.0f;
    int currentAttackIndex_ = 0; // 0: Left, 1: Right, 2: Mid
    bool hasFinishedAttack_ = false;

    enum class AttackPhase {
        WindUp,   // 溜め（体を圧縮）
        Sweep,    // 薙ぎ払い・振り下ろし
        Recovery, // 戻り
        Done
    };
    AttackPhase currentPhase_ = AttackPhase::WindUp;
    float phaseTimer_ = 0.0f;

    // --- ユーザ指定の調整用パラメータ ---
    float neckStretchScale_ = 3.0f; // 首を伸ばす（スケールYを伸ばす）
    float neckThickness_ = 1.5f;    // 首の太さ（スケールX/Zを太くする）
    float hitRangeScale_ = 1.2f;    // 範囲
    float sweepSpeed_ = 0.3f;       // 首を振る速さ補間（ゆっくりに）

    // --- 各フェーズの時間 ---
    float windUpTime_ = 1.2f;       // 溜め時間
    float sweepTime_ = 2.0f;       // 攻撃時間（遅く・重々しく）
    float recoveryTime_ = 2.0f;     // 戻り時間

    // --- 案2：体圧縮・屈折用パラメータ ---
    float bodySquashDepth_ = -2.5f; // 胴体をどれだけ下へ押し潰すか
    float bodyBendAmount_ = 3.5f;   // 胴体の横ズレ（くの字表現用）
    float bodyHeightOffset_ = 1.0f;   // 胴体全体をY軸でどれだけ上下にずらすか
    float attackHeadOffsetY_ = -3.5f; // 攻撃時、頭だけをどれだけ下へ下げるか
    float idleHeadOffsetY_ = -2.5f;   // 非攻撃時の頭の位置をどれだけ下げるか

    // --- 追従（トラッキング）パラメータ ---
    float trackSpeedWindUp_ = 1.0f;  // 振りかぶり時の接近速度
    float trackSpeedSweep_ = 10.0f;   // 攻撃時の突進速度
    float trackRotSpeed_ = 4.0f;     // 追従のための旋回速度
};
