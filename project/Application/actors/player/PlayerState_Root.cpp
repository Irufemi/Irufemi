#include "Math.h"  // Add などのユーティリティ
#include "Player.h"
#include "PlayerState.h"

#include "engine/Input/InputManager.h"


// 注意: Player から friend 許可を受けるため、無名名前空間ではなくグローバル定義でも可。
// ここでは衝突を避けつつ、ひとまずグローバルに直接定義する。
struct PlayerStateRoot final : IPlayerState {
	const char* Name() const override { return "Root"; }
	void Enter(Player&) override {}
	void Exit(Player&) override {}

	/// <summary>
	/// 通常状態：
	/// ・入力(左右/ジャンプ/重力)は Player::MoveInput() に集約
	/// ・ここでは「ダッシュへ遷移するか」を見るのみ
	/// </summary>
	void Update(Player& player) override {
		player.MoveInput();

		// ダッシュ開始トリガ(例: スペースキー or Xボタン)
		if (player.inputManager_->IsKeyPressed(VK_SPACE) || player.inputManager_->IsButtonPressed(XINPUT_GAMEPAD_X)) {
			// 地上なら何度でも、空中なら未使用時のみ許可
			if (player.onGround_ || !player.dashUsed_) {
				player.se_dash_->Play();
				// 空中で発動したら1回分使用したことにする
				if (!player.onGround_) {
					player.dashUsed_ = true;
				}
				player.ChangeState(MakeDashState());
			}
		}

		// 攻撃開始トリガ(例: Eキー or Bボタン)
		if (player.inputManager_->IsKeyPressed('E') || player.inputManager_->IsButtonPressed(XINPUT_GAMEPAD_B)) {
			player.ChangeState(MakeAttackState());
			player.se_slash_->Play();
		}
	}
};

std::unique_ptr<IPlayerState> MakeRootState() { return std::make_unique<PlayerStateRoot>(); }