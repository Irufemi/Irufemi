#pragma once

#include <memory>

class Player;

/// @struct IPlayerState
/// @brief Playerクラスの状態を管理するためのステートパターンのインターフェース
/// @details Playerの各状態（待機、ダッシュ、攻撃など）が持つべき共通の処理を定義します。
/// 具体的な振る舞いは、このインターフェースを継承した各ステートクラスで実装されます。
/// ・Enter: 状態に入った瞬間に1回
/// ・Update: 毎フレーム呼ばれる
/// ・Exit: 状態を抜ける瞬間に1回
/// </summary>
struct IPlayerState {
	virtual ~IPlayerState() = default;
	virtual const char* Name() const = 0;          // デバッグ表示用の状態名
	virtual bool IsDashing() const { return false; } // ダッシュ中か
	virtual bool IsAttacking() const { return false; } // 攻撃中か
	virtual void Enter(Player& player) = 0;        // 状態開始
	virtual void Update(Player& player) = 0;       // 毎フレーム更新
	virtual void Exit(Player& player) = 0;         // 状態終了
};

// 具体ステート生成関数(実体は cpp 側)
std::unique_ptr<IPlayerState> MakeRootState();
std::unique_ptr<IPlayerState> MakeDashState();
std::unique_ptr<IPlayerState> MakeAttackState();