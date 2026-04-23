#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

// 第1形態の全体を使った巨体タックル攻撃
class Phase1_Tackle : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return hasFinished_; }

private:
    enum class Phase {
        PreAttack,  // 押し潰し・首出し・シェイク
        Aim,        // エイム（プレイヤー方向を向く）
        Wait,       // 突進前のタメ（ロックオン完了後）
        Rush,       // 突進
        Stun,       // 壁にぶつかった時のスタン
        ReturnToIdle// 元の姿勢に戻る
    };

    Phase currentPhase_ = Phase::PreAttack;
    float stateTimer_ = 0.0f;
    float totalTimer_ = 0.0f;
    float effectTimer_ = 0.0f;
    bool hasFinished_ = false;

    // タックル回数管理
    int rushCount_ = 0;
    const int kMaxRushCount = 3;

    // 各フェーズの時間パラメータ
    const float kPreAttackTime = 1.0f; // 潰れる時間
    const float kAimTime = 0.3f;       // ロックオンする時間
    const float kWaitTime = 1.2f;      // ロックオン完了後のタメ時間（回避猶予）
    const float kRushTime = 1.5f;      // 最大突進時間（壁に当たるか時間切れまで）
    const float kStunTime = 5.0f;      // スタン時間
    const float kReturnTime = 1.5f;    // Idleへ戻る時のイージング時間

    // 突進パラメータ
    const float kRushSpeed = 3.0f;     // 突進の1フレームあたりの前進量
    const float kWallLimit = 90.0f;    // フィールドの壁判定ライン (abs(x) または abs(z))

    // 動きのパラメータ
    const float kSquashScale = 0.8f;   // 潰れ時のYスケール
    const float kNormalScale = 4.0f;   // 通常の全体のスケール幅 (globalTransform_.scale.y のベース)
    const float kShakeIntensity = 0.3f;// はげしいシェイクの強度
    const float kNeckExtension = 1.5f; // 首を前に出す距離 (Z軸)

    // ロックオン中の目標方向
    Vector3 rushDirection_ = { 0, 0, 0 };
    float targetRotateY_ = 0.0f;
};
