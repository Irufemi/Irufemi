#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief スタンプ（踏みつけ）攻撃ステート
 */
class Phase1_Stomp : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return hasFinishedAttack_; }

private:
    float attackTimer_ = 0.0f;
    bool hasFinishedAttack_ = false;
    bool hasTeleported_ = false;
    bool hasHitGround_ = false;
    float initialScaleY_ = 1.0f;
    float rotationInterpolationSpeed_ = 5.0f; 
    Vector3 targetPos_ = {}; // ストンプの目標落下地点 

    // --- 演出パラメータ ---
    float squatTime_ = 0.6f;        
    float holdTime_ = 1.2f;         
    float jumpTime_ = 0.35f;        
    float hoverTime_ = 2.5f;        
    float recoveryTime_ = 1.2f;     
    float landSquatScale_ = 0.35f;  
    float landSquatDownTime_ = 0.4f; 
    float landSquatHoldTime_ = 0.2f;  
    float landRiseTime_ = 3.0f;       

    float maxSquatScale_ = 0.5f;    
    float jumpStretchScale_ = 2.0f; 

    float squatShakeStrength_ = 0.5f; 
    float holdShakeStrength_ = 0.15f; 
    float holdShakeSpeed_ = 180.0f;   

    float jumpUpSpeed_ = 4.0f;      
    float dropSpeed_ = 1.5f;        
    float stompHeight_ = 40.0f;     
    float groundY_ = 3.0f;          
};