#pragma once
#include "../../IEnemyAnimationState.h"
#include "Tackle/Phase2_Tackle.h"
#include "Beam/Phase2_Beam.h"
#include "Idle/Phase2_Idle.h"
#include "core/math/Vector3.h"
#include "core/math/Transform.h"
#include <array>
#include <memory>

/**
 * @brief 第2形態（首の独立浮遊）ステート・マネージャー
 * 3つの首の独立したステート（Idle, Tackling, Beaming）を管理します。
 */
class Phase2 : public IEnemyAnimationState {
public:
    void Enter(Enemy* enemy) override;
    void Update(Enemy* enemy, Player* player, float deltaTime) override;
    void Exit(Enemy* enemy) override;

    bool IsFinished() const override { return false; }

private:
   enum class Mode {
        Idle,
        Tackling,
        Beaming
    };
    std::array<Mode, 3> currentModes_;

    std::array<std::unique_ptr<Phase2_Idle>, 3> idleStates_;
    std::array<std::unique_ptr<Phase2_Beam>, 3> beamStates_;
    std::array<std::unique_ptr<Phase2_Tackle>, 3> tackleStates_;

    // 重なり防止（反発）パラメータ
    const float kRepulsionRadius = 8.0f;    // 首同士が反発し始める距離 (m)
    const float kRepulsionForceScale = 0.1f; // 反発力の基本倍率

    void ApplyRepulsion(Enemy* enemy);
};
