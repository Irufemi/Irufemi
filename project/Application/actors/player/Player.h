#pragma once

#include <memory>

#include "math/shape/OBB.h"
#include "math/Vector3.h"
#include "math/Transform.h"

// 前方宣言
class Camera;
class ObjClass;
class InputManager;
class Sword; // forward

#include "audio/Se.h"

class Player {
public:
    Player();
    ~Player();
    void Initialize(Camera* camera, const Vector3 & pos, InputManager* input);
    void Update();
    void Draw();

    void UpdateOBB();

    // OBB の取得
    const OBB& GetOBB() const;

    // 衝突時の処理
    void HandleCollision();

    // expose sword for collision checks
    Sword* GetSword() { return sword_.get(); }

    // stun query
    bool IsStunned() const { return isStunned_; }

    // externally set stun duration (seconds)
    void StunFor(float seconds) { isStunned_ = true; stunTimer_ = seconds; }

private:

    OBB obb_{};

    Vector3 velocity = {};
    Vector3 lastSafePosition_ = {};

    float width_ = 2.0f;

    float height_ = 2.0f;

    float depth_ = 2.0f;

    static inline const float kAcceleration = 0.2f;


    //攻撃範囲関係
    bool attackRangeVisible_ = false;

    float attackRangeTimer_ = 0.0f;

    static inline constexpr float kAttackRangeDuration = 0.5f;

    float attackRangeBase_ = 2.0f;

    float attackRangeDistance_ = 2.0f;

    float attackRangeMax_ = 12.0f;

    float attackRangeModelTipOffset_ = 1.0f;

    // 尖端がモデル原点からどちらの方向にあるか（+1 または -1）
    float attackRangeModelTipDirection_ = -1.0f;

    // 尖端を固定するアンカ距離(プレイヤーからの固定した先端位置)。
    float attackRangeTipAnchorDistance_ = 2.0f;

    // 攻撃中フラグ
    bool isAttacking_ = false;
    // チャージ中フラグ
    bool isCharging_ = false;
    float chargeTimer_ = 0.0f;
    static inline constexpr float kMaxChargeTime = 2.0f; // 最大チャージ時間

private:

    std::unique_ptr<ObjClass> model_ = nullptr;//Playerのモデル
    std::unique_ptr<ObjClass> attackRangeModel_ = nullptr;//攻撃範囲表示用モデル
    std::unique_ptr<Sword> sword_ = nullptr; // sword model


    Transform transform_;

    Camera* camera_ = nullptr;

    InputManager* input_ = nullptr;

    // スタン関連
    bool isStunned_ = false;
    float stunTimer_ = 0.0f;
    static inline constexpr float kStunDuration = 2.0f; // スタン時間(秒)

private:
    void Move();

    void CreateObj(Camera* camera);

    void Attack();

  
    Se seCharge_;

    
    float chargeVolume_ = 0.0f;         
    float targetChargeVolume_ = 0.0f;  
    float volumeLerpSpeed_ = 3.0f;    
    bool pendingStopChargeSound_ = false; 

   
    bool chargeSoundStarted_ = false;
    float chargeSoundStartThreshold_ = 0.08f; 
};
