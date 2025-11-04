#pragma once

#include "math/shape/AABB.h"
#include "MapChipField.h" // IndexSet/Rect を参照するためヘッダで include
#include "PlayerState.h" // unique_ptr<IPlayerState> をメンバに持つため
#include "math/Vector3.h"
#include "3D/ObjClass.h"
#include <cstdint>
#include <memory>

// 前方宣言
class Enemy;
class Camera;
class InputManager;
struct IPlayerState;
struct PlayerStateRoot;
struct PlayerStateAttack;

/// <summary>
/// 自キャラ（State パターン導入版）
/// - 入力/移動/重力/衝突は Player 内の共通処理
/// - 行動（通常・攻撃など）の“意思決定と見た目”は State に分離
/// </summary>
class Player {
public: // ===== Public API =====
	// 左右向き
	enum class LRDirection { kRight, kLeft };

	// --- ライフサイクル ---
	void Initialize(ObjClass* model, Camera* camera,InputManager *inputManager, Vector3& position);
	void Update();
	void Draw();

	// --- ゲーム連携 ---
	void OnCollision(const Enemy* enemy);
	void SetMapChipField(MapChipField* mapChipField) { this->mapChipField_ = mapChipField; }

	// --- 状態取得（読み取り専用） ---
	const Vector3& GetVelocity() const { return this->velocity_; }
	const Vector3& GetTranslate() const { return model_->GetPosition(); }
	Vector3 GetWorldPosition();
	AABB GetAABB();
	LRDirection GetLR() const { return lrDirection_; }
	bool IsDead() const { return isDead_; }
	bool IsAttack() const; // 現在の状態が攻撃中か
	const char* GetStateName() const { return state_ ? state_->Name() : "<none>"; }

	// --- ステート制御 ---
	void ChangeState(std::unique_ptr<IPlayerState> next);

private: // ===== 内部型・定数 =====
	/// <summary>
	/// マップ衝突判定で使う一時情報
	/// </summary>
	struct CollisionMapInfo {
		bool isContactCeiling = false;      // ↑方向（頭）で天井にヒット
		bool isContactGround = false;       // ↓方向（足）で地面にヒット
		bool isContactWall = false;         // ←→方向で壁にヒット
		Vector3 amountMove{}; // 軸分離でクリップ後の最終移動量
	};

	/// <summary>プレイヤ AABB の角（X-Y 平面上）</summary>
	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	// --- チューニング用パラメータ（マリオ寄りの初期値） ---
	static inline const float kAcceleration = 0.018f;        // 地上: 横加速度
	static inline const float kAttenuation = 0.10f;          // 地上: 無入力減衰
	static inline const float kLimitRunSpeed = 0.30f;        // 地上: 最高速
	static inline const float kAirAcceleration = 0.011f;     // 空中: 横加速度
	static inline const float kAirAttenuation = 0.02f;       // 空中: 無入力減衰
	static inline const float kgravityAcceleration = 0.010f; // 重力
	static inline const float kLimitFallSpeed = 0.36f;       // 落下終端速度
	static inline const float kFallGravityScale = 1.2f;      // 下降時の重力倍率
	static inline const float kJumpAcceleration = 0.28f;     // ジャンプ初速
	static inline const float kTimeTurn = 0.18f;             // 旋回演出時間
	static inline const float kWidth = 0.8f;                 // 当たり判定 幅
	static inline const float kHeight = 0.8f;                // 当たり判定 高さ
	static inline const float kMBlank = 0.01f;               // めり込み防止マージン
	static inline const float kAttenuationLanding = 0.08f;   // 着地時の水平減衰
	static inline const float kAttenuationWall = 0.25f;      // 壁接触時の水平減衰
	static inline const float kJumpCutFactor = 0.5f;         // ジャンプ短押しカット倍率
	static inline const int kCoyoteFrames = 6;               // コヨーテタイム（フレーム）
	static inline const int kJumpBufferFrames = 6;           // ジャンプバッファ（フレーム）
	static inline const Vector3 kattackVelocity_{0.4f, 0.0f, 0.0f};

private: // ===== データメンバ =====
	// ステート
	std::unique_ptr<IPlayerState> state_{};

	// 移動関連
	Vector3 velocity_{};              // 速度（フレーム単位）
	bool onGround_ = true;                          // 接地中か
	LRDirection lrDirection_ = LRDirection::kRight; // 向き
	float turnFirstRotationY_ = 0.0f;               // 旋回開始角
	float turnTimer_ = 0.0f;                        // 旋回残り時間

	// 入力補助
	int coyoteCounter_ = 0;     // コヨーテタイムカウンタ
	int jumpBufferCounter_ = 0; // ジャンプバッファカウンタ

	// 変換・描画
	ObjClass* model_ = nullptr;
	Camera* camera_ = nullptr;
	InputManager* inputManager_ = nullptr;

	// マップ
	MapChipField* mapChipField_ = nullptr;

	// 生存
	bool isDead_ = false;

private: // ===== 内部処理 =====
	// 入力/移動
	void MoveInput();
	void BehaviorMoveUpdate();
	void TurningControl(); // 見た目の向き補間
	void UpdateMatrix();

	// 衝突
	void CollisionDetection(CollisionMapInfo& info);
	void MoveAccordingly(const CollisionMapInfo& info);
	void ContactCeiling(const CollisionMapInfo& info);
	void ContactGround(const CollisionMapInfo& info);
	void ContactWall(const CollisionMapInfo& info);

	// 幾何/ユーティリティ
	Vector3 CornerPosition(const Vector3& center, Corner corner);
	bool IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const;
	float ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const;
	float ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const;

	// ステートから private へアクセスを許可
	friend struct IPlayerState;
	friend struct PlayerStateRoot;
	friend struct PlayerStateAttack;

	// 旧個別判定（参考用・未使用）
	void MapCollisionTop(CollisionMapInfo& info);
	void MapCollisionBottom(CollisionMapInfo& info);
	void MapCollisionRight(CollisionMapInfo& info);
	void MapCollisionLeft(CollisionMapInfo& info);
};