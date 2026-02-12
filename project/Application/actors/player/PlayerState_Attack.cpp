#include "PlayerState.h"
#include "Player.h"
#include "PlayerPhysics.h" // 追加
#include "function/Ease.h"
#include "function/Math.h"
#include <numbers>

/// <summary>
/// 攻撃状態
/// ・一定時間で Root 状態に復帰する
/// </summary>
struct PlayerStateAttack final : IPlayerState {
    int timer_ = 0;
    static const int kAttackDuration = 15; // 攻撃持続フレーム
    Vector3 attackStartPosition_{}; // 攻撃開始時のエフェクト位置

    const char* Name() const override { return "Attack"; }
    bool IsAttacking() const override { return true; }

    void Enter(Player& player) override {
        timer_ = 0;
        player.physics_->SetVelocity({ 0.0f, player.GetVelocity().y, 0.0f });
        player.attackEffectTransform_.translate = player.transform_.translate;
        attackStartPosition_ = player.transform_.translate;
        player.attackEffectTransform_.rotate.y = (player.GetLR() == LRDirection::kRight)
            ? std::numbers::pi_v<float> / 2.0f
            : -std::numbers::pi_v<float> / 2.0f;
    }

    void Update(Player& player) override {
        ++timer_;
        player.physics_->ApplyGravity();

        float t = static_cast<float>(timer_) / kAttackDuration;
        const float attackRange = 1.5f;
        float moveDirection = (player.GetLR() == LRDirection::kRight) ? 1.0f : -1.0f;
        Vector3 endPosition = attackStartPosition_;
        endPosition.x += attackRange * moveDirection;
        player.attackEffectTransform_.translate = Lerp(attackStartPosition_, endPosition, EaseOutSine(t));

        if (timer_ >= kAttackDuration) {
            player.ChangeState(MakeRootState());
        }
    }

    void Exit(Player& player) override {
    }
};

std::unique_ptr<IPlayerState> MakeAttackState() {
    return std::make_unique<PlayerStateAttack>();
}