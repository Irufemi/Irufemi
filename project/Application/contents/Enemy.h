#pragma once

#include "KamataEngine.h"

#include "AABB.h"

// 前方宣言
class Player;
class GameScene;

/// <summary>
/// 敵
/// </summary>
class Enemy {
private: // メンバ変数
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;

	// モデル
	KamataEngine::Model* model_ = nullptr;

	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

    // 速度
	KamataEngine::Vector3 velocity_{};
	static inline const float kWalkSpeed = 0.05f;

    // 最初の角度[度]
	static inline const float kWalkMotionAngleStart = -0.5f;
    // 最後の角度[度]
	static inline const float kWalkMotionAngleEnd = 0.5f;
    // アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 1.0f;

    // 経過時間
	float walkTimer = 0.0f;

    // 敵の当たり判定サイズ
	static inline const float kWidth = 1.0f;
	static inline const float kHeight = 1.0f;

    // デスフラグ
	bool isDead_ = false;

	// 振る舞い
	enum class Behavior {
		// 歩行状態
		kWalk,
		// デス中
		kDeath,
		// 変更なし
		kUnknown,
	};

	// 振る舞い
	Behavior behavior_ = Behavior::kWalk;
	// 振る舞いリクエスト
	Behavior behaviorRequest_ = Behavior::kUnknown;

	// デス中のアニメーションタイマー
	float deathTimer_ = 0.0f;

	// デス演出の時間
	static inline const float kDeathDuration = 0.6f;

	// デス演出が始まるときの角度
	KamataEngine::Vector3 deathStartRotation_{};
	// デス演出が終わるときの角度
	KamataEngine::Vector3 deathEndRotation_{};
	
	// 左右
	enum class LRDirection {
		kRight, // 右
		kLeft,  // 左
	};

	LRDirection lrDirection_ = LRDirection::kLeft;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間(秒)
	static inline const float kTimeTurn = 0.6f;

	// 旋回しているか
	bool isTurn_ = false;

	// コリジョンを無効化するか
	bool isCollisionDisabled_ = false;

	// ゲームシーン(借りてくる用)
	GameScene* gameScene_ = nullptr;


public: // メンバ関数

    /// <summary>
    /// 初期化
    /// </summary>
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, KamataEngine::Vector3& position, GameScene* gameScene);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

	/// <summary>
	/// 歩行行動初期化
	/// </summary>
	void BehaviorWalkInitialize();

	/// <summary>
	/// デス行動初期化
	/// </summary>
	void BehaviorDeathInitialize();

	/// <summary>
	/// 歩行行動更新
	/// </summary>
	void BehaviorWalkUpdate();

	/// <summary>
	/// デス行動更新
	/// </summary>
	void BehaviorDeathUpdate();

    /// <summary>
	/// ワールド座標を取得
	/// </summary>
	/// <returns></returns>
	KamataEngine::Vector3 GetWorldPosition();

	// 旋回制御
	void TurningControl();

    // AABBを取得
	AABB GetAABB();

	// デスフラグのgetter
	bool IsDead() const { return isDead_; }

    // Playerとの衝突時関数
	void OnCollision(const Player* player);

	bool IsCollisionDisabled() const { return isCollisionDisabled_; }
};
