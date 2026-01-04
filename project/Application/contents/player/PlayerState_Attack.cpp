#include "PlayerState.h"
#include "Player.h"

/// <summary>
/// 攻撃状態
/// ・一定時間で Root 状態に復帰する
/// </summary>
struct PlayerStateAttack final : IPlayerState {
    int timer_ = 0;
    static const int kAttackDuration = 15; // 攻撃持続フレーム

    const char* Name() const override { return "Attack"; }
    bool IsAttacking() const override { return true; }

    void Enter(Player& player) override {
        timer_ = 0;
        // 攻撃中は移動を停止
        player.velocity_ = { 0.0f, player.velocity_.y, 0.0f };
    }

    void Update(Player& player) override {
        ++timer_;
        // プレイヤーの重力は通常通り適用
        player.ApplyGravity();

        if (timer_ >= kAttackDuration) {
            player.ChangeState(MakeRootState());
        }
    }

    void Exit(Player& player) override {
        // 状態を抜けるときに後処理があれば記述
    }
};

std::unique_ptr<IPlayerState> MakeAttackState() {
    return std::make_unique<PlayerStateAttack>();
}