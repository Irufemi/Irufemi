#include "PlayerState.h"
#include "Player.h"
#include "function/Ease.h" // Ease関数を使うためにインクルード
#include "function/Math.h"  // Lerp関数を使うためにインクルード
#include <numbers> // std::numbers::pi_v を使うためにインクルード

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
        // 攻撃中は移動を停止
        player.velocity_ = { 0.0f, player.velocity_.y, 0.0f };
        // 攻撃エフェクトの初期位置をプレイヤーに合わせる
        player.attackEffectTransform_.translate = player.transform_.translate;
        attackStartPosition_ = player.transform_.translate;
        // プレイヤーの向きに合わせて攻撃エフェクトの向きを設定
        player.attackEffectTransform_.rotate.y = (player.lrDirection_ == LRDirection::kRight)
            ? std::numbers::pi_v<float> / 2.0f
            : -std::numbers::pi_v<float> / 2.0f;
    }

    void Update(Player& player) override {
        ++timer_;
        // プレイヤーの重力は通常通り適用
        player.ApplyGravity();

        // --- 攻撃エフェクトのアニメーション ---
        float t = static_cast<float>(timer_) / kAttackDuration;
        // 攻撃範囲（例として1.5f）
        const float attackRange = 1.5f;
        // プレイヤーの向きに合わせて移動方向を決める
        float moveDirection = (player.lrDirection_ == LRDirection::kRight) ? 1.0f : -1.0f;
        // 開始位置から目標位置へ移動
        Vector3 endPosition = attackStartPosition_;
        endPosition.x += attackRange * moveDirection;

        // イージングをかけて位置を更新
        player.attackEffectTransform_.translate = Lerp(attackStartPosition_, endPosition, EaseOutSine(t));


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