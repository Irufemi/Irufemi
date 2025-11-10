#include "PlayerState.h"
#include "Player.h"
#include "function/Ease.h"
#include "function/Math.h"

/// <summary>
/// 攻撃状態：ため(Charge)→突進(Tackle)→余韻(Aftereffect)
/// 見た目（スケール変化）と速度の付与だけを担当し、
/// 物理・衝突は Player 側の共通処理に任せる。
/// </summary>
struct PlayerStateAttack final : IPlayerState {
	enum class Phase { Charge, Tackle, Aftereffect };
	int timer_ = 0;
	Phase phase_ = Phase::Charge;

	const char* Name() const override { return "Attack"; }
	bool IsAttack() const override { return true; }

	void Enter(Player& player) override {
		//timer_ = 0;
		//phase_ = Phase::Charge;
		//// 速度リセット＆攻撃モデルの姿勢をプレイヤに合わせる
		//player.velocity_ = {0.0f, 0.0f, 0.0f};
		//player.worldTransformAttack_.translation_ = player.worldTransform_.translation_;
		//player.worldTransformAttack_.rotation_ = player.worldTransform_.rotation_;
	}

	void Update(Player& player) override {
		//++timer_;
		//switch (phase_) {
		//case Phase::Charge: {
		//	// ため（縦に伸ばし、奥行きを絞る）
		//	float t = static_cast<float>(timer_) / 2.0f;
		//	player.worldTransform_.scale_.z = Lerp(1.0f, 0.3f, EaseOutSine(t));
		//	player.worldTransform_.scale_.y = Lerp(1.0f, 1.6f, EaseOutSine(t));
		//	if (timer_ >= 2) {
		//		phase_ = Phase::Tackle;
		//		timer_ = 0;
		//	}
		//	break;
		//}
		//case Phase::Tackle: {
		//	// 突進（縦を絞って奥行きを伸ばす）＋ 向きに応じて速度付与
		//	float t = static_cast<float>(timer_) / 10.0f;
		//	player.worldTransform_.scale_.z = Lerp(0.3f, 1.3f, EaseOutSine(t));
		//	player.worldTransform_.scale_.y = Lerp(1.6f, 0.7f, EaseInSine(t));
		//	if (timer_ >= 10) {
		//		phase_ = Phase::Aftereffect;
		//		timer_ = 0;
		//	}

		//	const auto v = (player.lrDirection_ == Player::LRDirection::kRight) ? Player::kattackVelocity_ : Math::Multiply(-1.0f, Player::kattackVelocity_);
		//	player.velocity_ = v;
		//	break;
		//}
		//case Phase::Aftereffect: {
		//	// 余韻（スケールを元へ戻す）→ Root へ復帰
		//	float t = static_cast<float>(timer_) / 2.0f;
		//	player.worldTransform_.scale_.z = Lerp(1.3f, 1.0f, EaseOutSine(t));
		//	player.worldTransform_.scale_.y = Lerp(0.7f, 1.0f, EaseOutSine(t));
		//	if (timer_ >= 2) {
		//		player.worldTransform_.scale_.z = 1.0f;
		//		player.worldTransform_.scale_.y = 1.0f;
		//		player.ChangeState(MakeRootState());
		//	}
		//	break;
		//}
		//}
	}

	void Exit(Player&) override {}
};

std::unique_ptr<IPlayerState> MakeAttackState() { return std::make_unique<PlayerStateAttack>(); }