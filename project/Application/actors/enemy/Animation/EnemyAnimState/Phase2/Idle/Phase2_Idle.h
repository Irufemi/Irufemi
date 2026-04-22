#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 第2形態の浮遊・徘徊ステート
 */
class Phase2_Idle : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void SetHeadIndex(int index) { headIndex_ = index; }
    
    // 状態遷移の要求フラグ
    bool WantsToBite() const { return wantsToBite_; }
    bool WantsToBeam() const { return wantsToBeam_; }

    // Phase2側から速度を参照・適用するため
    Vector3& GetVelocity() { return velocity_; }
    void SetVelocity(const Vector3& vel) { velocity_ = vel; }

    bool IsFinished() const override { return false; } // Idleから自動終了はしない

private:
    int headIndex_ = 1;
    float timer_ = 0.0f;
    float globalTimer_ = 0.0f;

    Vector3 velocity_ = { 0, 0, 0 };
    Vector3 wanderTarget_ = { 0, 0, 0 };

    bool wantsToBite_ = false;
    bool wantsToBeam_ = false;

    // 個別パラメータ
    float orbitSpeed_ = 0.3f;
    float springStrength_ = 0.02f;
    float friction_ = 0.82f;

    // 攻撃再開までのクールダウン時間
    float cooldownTime_ = 1.0f; 

    // 高度制御
    const float kLowHeight = 3.5f;        
    const float kHighHeight = 15.0f;      
    const float kHeightChangeDistMin = 15.0f; 
    const float kHeightChangeDistMax = 35.0f; 

    // 移動速度・挙動
    const float kSpeedMultiplier = 0.22f;  
    const float kFieldLimit = 90.0f;       
    const float kWanderArrivalDist = 8.0f; 

    // 攻撃開始基準
    const float kBiteDistThreshold = 22.0f;  
    const float kBiteCooldown = 4.0f;       
    const float kBeamDistThreshold = 30.0f;  
    const float kBeamCooldown = 7.0f;       
};
