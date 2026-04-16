#pragma once
#include "IEnemyAnimationState.h"
#include "core/math/Vector3.h"
#include "core/math/Transform.h"
#include <vector>
#include <array>

/**
 * @brief 第2形態（首の独立浮遊）ステート
 * 3つの首が個別に徘徊し、プレイヤーとの距離に応じて高度や攻撃パターンを変化させます。
 */
class EnemyAnimState_Phase2 : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

private:
   struct HeadState {
        Vector3 velocity = { 0, 0, 0 };
        float behaviorTimer = 0.0f;
        
        enum class Mode {
            Floating,    // 徘徊・追尾
            Biting,      // カミツキ攻撃
            Beaming,     // ビーム攻撃
            Recovering   // 攻撃後の復帰待機
        } mode = Mode::Floating;

        Vector3 attackTarget = { 0, 0, 0 };
        Vector3 wanderTarget = { 0, 0, 0 };
        
        // 各首ごとの固有パラメータ
        float orbitSpeed = 0.3f;
        float springStrength = 0.02f;
        float friction = 0.95f;
    };

    // --- 調整用パラメータ群（マジックナンバー排除） ---

    // 高度制御
    const float kLowHeight = 3.5f;        // プレイヤーに近い時の高度 (m)
    const float kHighHeight = 15.0f;      // プレイヤーから遠い時の高度 (m)
    const float kHeightChangeDistMin = 15.0f; // この距離より近ければ最低高度
    const float kHeightChangeDistMax = 35.0f; // この距離より遠ければ最高高度

    // 移動速度・挙動
    const float kSpeedMultiplier = 0.22f;  // 全体的な移動の素早さ係数（低いほどどっしり動く）
    const float kFrictionBase = 0.82f;     // ブレーキの強さ（低いほど急に止まる）
    const float kFieldLimit = 90.0f;       // フィールドの移動制限範囲 (±m)
    const float kWanderArrivalDist = 8.0f; // 目的地に到着したとみなす距離 (m)

    // 攻撃分岐
    const float kBiteDistThreshold = 22.0f;  // カミツキを開始する最大距離 (m)
    const float kBiteCooldown = 4.0f;       // カミツキの最低インターバル (sec)
    const float kBeamDistThreshold = 30.0f;  // ビームを開始する最小距離 (m)
    const float kBeamCooldown = 7.0f;       // ビームの最低インターバル (sec)

    // 重なり防止（反発）
    const float kRepulsionRadius = 8.0f;    // 首同士が反発し始める距離 (m)
    const float kRepulsionForceScale = 0.1f; // 反発力の基本倍率

    // -------------------------------------------

    std::array<HeadState, 3> headStates_;
    float globalTimer_ = 0.0f;

    void UpdateFloating(int index, HeadState& state, Enemy* enemy, Player* player, float deltaTime);
    void UpdateBiting(int index, HeadState& state, Enemy* enemy, Player* player, float deltaTime);
    void UpdateBeaming(int index, HeadState& state, Enemy* enemy, Player* player, float deltaTime);

    // 重なり防止
    void ApplyRepulsion(Enemy* enemy);
};
