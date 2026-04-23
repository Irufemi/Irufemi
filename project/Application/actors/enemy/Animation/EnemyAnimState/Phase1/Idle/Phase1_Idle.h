#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 待機ステート
 */
class Phase1_Idle : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override {}
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override {}
    bool IsFinished() const override { return true; }

private:
    float timer_ = 0.0f;
    float creepSpeed_ = 1.0f;       // じりじり寄る速度
    float trackRotSpeed_ = 2.0f;    // プレイヤーを向く旋回速度

    float breathSpeed_ = 2.0f;
    float breathHeight_ = 0.25f;    // 呼吸の上下
    
    // 首振りのパラメータ（歩きモーション）
    float headSwingSpeed_ = 3.0f;   
    float headSwingDepth_ = 1.0f;   // 前後（Z軸）への振幅
    float phaseOffset_ = 2.09f;     // 3つの頭をずらす位相 (2π/3 ≒ 2.09)

    // くねくね
    float bodyWiggleSpeed_ = 4.0f;
    float bodyWiggleWidth_ = 0.5f;

    float lerpSpeed_ = 0.1f;
    float returnSpeed_ = 0.03f;
};