#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief カミツキ攻撃ステート
 * 第1形態（ボス全体）および第2形態（各首）の両方で使用可能な設計。
 */
class Bite : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    /**
     * @brief どの首を動かすかを設定（第2形態用）
     * @param index 0:Left, 1:Mid, 2:Right
     */
    void SetHeadIndex(int index) { headIndex_ = index; }

    bool IsFinished() const override { return isFinished_; }

private:
    float timer_ = 0.0f;
    bool isFinished_ = false;
    int headIndex_ = 1; // デフォルトはMid首

    // 調整用パラメータ
    const float kChargeTime = 0.8f;      // 溜め時間
    const float kRushTime = 1.2f;        // 突進終了までの時間
    const float kRushSpeed = 0.8f;       // 突進速度
    const float kBackStepSpeed = 0.04f;   // 溜め時の後退速度
};
