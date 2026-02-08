#include "PlayerState.h"
#include "Player.h"
#include "PlayerPhysics.h" // 追加
#include "function/Ease.h"
#include "function/Math.h"
#include "3D/ObjClass.h"

struct PlayerStateDash final : IPlayerState {
	enum class Phase { Charge, Dash, Aftereffect };
	int timer_ = 0;
	Phase phase_ = Phase::Charge;

	const char* Name() const override { return "Dash"; }
	bool IsDashing() const override { return true; }

	void Enter(Player& player) override {
		timer_ = 0;
		phase_ = Phase::Charge;
		player.physics_->SetVelocity({ 0.0f, 0.0f, 0.0f });
		player.model_->SetAlpha(0.5f);
	}

	void Update(Player& player) override {
		++timer_;
		switch (phase_) {
		case Phase::Charge: {
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
			float t = static_cast<float>(timer_) / 10.0f;
			player.transform_.scale.x = Lerp(0.8f, 1.5f, EaseOutSine(t));
			player.transform_.scale.y = Lerp(0.8f, 0.7f, EaseInSine(t));
			if (timer_ >= 10) {
				phase_ = Phase::Aftereffect;
				timer_ = 0;
			}
			const auto v = (player.GetLR() == LRDirection::kRight) ? Player::kDashVelocity_ : Math::Multiply(-1.0f, Player::kDashVelocity_);
			player.physics_->SetVelocity(v);
			break;
		}
		case Phase::Aftereffect: {
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
		player.model_->SetAlpha(1.0f);
		player.transform_.scale = { 1.0f, 1.0f, 1.0f };
	}
};

std::unique_ptr<IPlayerState> MakeDashState() { return std::make_unique<PlayerStateDash>(); }