#pragma once

#include <memory>

class Player;

/// <summary>
/// Player のステート共通インターフェース
/// ・Enter: 状態に入った瞬間に1回
/// ・Update: 毎フレーム呼ばれる
/// ・Exit: 状態を抜ける瞬間に1回
/// </summary>
struct IPlayerState {
	virtual ~IPlayerState() = default;
	virtual const char* Name() const = 0;           // デバッグ表示用の状態名
	virtual bool IsAttack() const { return false; } // 攻撃モデルを描画すべきか
	virtual void Enter(Player& player) = 0;         // 状態開始
	virtual void Update(Player& player) = 0;        // 毎フレーム更新
	virtual void Exit(Player& player) = 0;          // 状態終了
};

// 具体ステート生成関数（実体は cpp 側）
std::unique_ptr<IPlayerState> MakeRootState();
std::unique_ptr<IPlayerState> MakeAttackState();