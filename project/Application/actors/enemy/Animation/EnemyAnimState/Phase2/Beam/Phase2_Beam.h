#pragma once
#include "../../../IEnemyAnimationState.h"
#include "core/math/Vector3.h"

/**
 * @brief 第2形態のビーム攻撃ステート
 */
class Phase2_Beam : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    void SetHeadIndex(int index) { headIndex_ = index; }
    bool IsFinished() const override { return isFinished_; }

private:
    float timer_ = 0.0f;
    float globalTimer_ = 0.0f;
    bool isFinished_ = false;
    bool isLockedOn_ = false;
    bool hasPlayedBeamSe_ = false;
    int headIndex_ = 1;         // 管理する首のインデックス

    // タイマー・座標系
    Vector3 attackTarget_ = { 0, 0, 0 };
    Vector3 basePos_ = { 0, 0, 0 }; // シェイクの中心座標

    // --- 調整用パラメータ ---
    // 時間
    const float kBeamChargeTime = 1.8f;      // チャージ・シェイク
    const float kBeamWaitTime = 2.4f;        // 停止（ロックオン）
    const float kBeamFireTime = 3.9f;        // 本射終了

    // シェイク（震え）
    float shakeSpeedCharge_ = 80.0f;         // チャージ中のシェイク速度
    float shakeSpeedFire_ = 120.0f;          // 本射中のシェイク速度
    float kBeamShakeStrength = 0.3f;         // チャージ中のシェイク揺れ幅
    float fireShakeStrength_ = 0.01f;        // 本射中のシェイク揺れ幅

    // ビーム太さ・見た目
    float headExtensionY_ = 16.0f;           // 頭の中心からのビーム発射口の高さオフセット
    float telegraphThicknessBase_ = 1.0f;    // 予告線の基本の太さ
    float telegraphThicknessGrow_ = 1.6f;    // 予告線の太さの増加率
    float telegraphThicknessWait_ = 4.0f;    // 発射直前のロックオン時の予告線の太さ
    float attackThickness_ = 4.0f;          // 本射ビームの太さ
    float beamExpandScale_ = 2.5f;           // 後半の膨張スケール
    float fadeOutStartThreshold_ = 0.85f;    // 膨張開始のしきい値
};
