#include "PlayerState.h"
#include "Player.h"
#include "function/Ease.h"
#include "function/Math.h"
#include "3D/ObjClass.h"

/// <summary>
/// ダッシュ状態：溜め(Charge)→突進(Dash)→余韻(Aftereffect)
/// 見た目(スケールとアルファ値)と速度の付与だけを担当し、
/// 物理・衝突は Player 側の共通処理に任せる。
/// </summary>
struct PlayerStateDash final : IPlayerState {
	enum class Phase { Charge, Dash, Aftereffect };
	int timer_ = 0;
	Phase phase_ = Phase::Charge;

	const char* Name() const override { return "Dash"; }
	bool IsDashing() const override { return true; }

	void Enter(Player& player) override {
		timer_ = 0;
		phase_ = Phase::Charge;
		// 速度リセット
		player.velocity_ = { 0.0f, 0.0f, 0.0f };
		// ダッシュ中は半透明にする
		player.model_->SetAlpha(0.5f);
	}

	void Update(Player& player) override {
		++timer_;
		switch (phase_) {
		case Phase::Charge: {
			// 溜め(少し縮む)
			float t = static_cast<float>(timer_) / 2.0f;
			player.transform_.scale.x = Lerp(1.0f, 0.8f, EaseOutSine(t));
			player.transform_.scale.y = Lerp(1.0f, 0.8f, EaseOutSine(t));
			if (timer_ >= 2) {
				phase_ = Phase::Dash;
				timer_ = 0;
			}
			break;
		}
		case Phase::Dash: {
			// 突進(横に伸びる)＋ 向きに応じて速度付与
			float t = static_cast<float>(timer_) / 10.0f;
			player.transform_.scale.x = Lerp(0.8f, 1.5f, EaseOutSine(t));
			player.transform_.scale.y = Lerp(0.8f, 0.7f, EaseInSine(t));
			if (timer_ >= 10) {
				phase_ = Phase::Aftereffect;
				timer_ = 0;
			}

			const auto v = (player.lrDirection_ == LRDirection::kRight) ? Player::kDashVelocity_ : Math::Multiply(-1.0f, Player::kDashVelocity_);
			player.velocity_ = v;
			break;
		}
		case Phase::Aftereffect: {
			// 余韻(スケールを元へ戻す)→ Root へ復帰
			float t = static_cast<float>(timer_) / 8.0f;
			player.transform_.scale.x = Lerp(1.5f, 1.0f, EaseOutSine(t));
			player.transform_.scale.y = Lerp(0.7f, 1.0f, EaseOutSine(t));
			if (timer_ >= 8) {
				player.transform_.scale = { 1.0f, 1.0f, 1.0f };
				player.ChangeState(MakeRootState());
			}
			break;
		}
		}
	}

	void Exit(Player& player) override {
		// 状態を抜けるときに必ずアルファ値を元に戻す
		player.model_->SetAlpha(1.0f);
		player.transform_.scale = { 1.0f, 1.0f, 1.0f };
	}
};

std::unique_ptr<IPlayerState> MakeDashState() { return std::make_unique<PlayerStateDash>(); }