#pragma once

#include "math/shape/AABB.h"
#include "contents/MapChipField.h"
#include "PlayerState.h"
#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/Matrix4x4.h"
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

class Player {
public:
	enum class LRDirection { kRight, kLeft };

	// --- ライフサイクル ---
	void Initialize(ObjClass* model, Camera* camera, InputManager* inputManager, Vector3& position);
	void Update();
	void Draw();

	// --- ゲーム連携 ---
	void OnCollision(const Enemy* enemy);
	void SetMapChipField(MapChipField* mapChipField) { this->mapChipField_ = mapChipField; }

	// --- 状態取得（読み取り専用） ---
	const Vector3& GetVelocity() const { return this->velocity_; }
	const Vector3& GetTranslate() const { return transform_.translate; }
	Vector3 GetWorldPosition();
	AABB GetAABB();
	LRDirection GetLR() const { return lrDirection_; }
	bool IsDead() const { return isDead_; }
	bool IsAttack() const;
	const char* GetStateName() const { return state_ ? state_->Name() : "<none>"; }
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Transform& GetTransform() const { return transform_; }

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
		int  wallDir = 0;                   // 壁の在る側: +1=右壁, -1=左壁, 0=なし
		Vector3 amountMove{};               // 軸分離でクリップ後の最終移動量
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
	// 追加: 空中で使える追加ジャンプ回数（2段ジャンプなら1）
	static inline const int kMaxAirJumps = 1;

	// 追加: 壁ジャンプ用パラメータ
	static inline const float kWallJumpHorizontal = 0.26f;   // 壁から離れる水平速度
	static inline const float kWallJumpVertical = 0.28f;     // 壁ジャンの上向き速度
	static inline const int   kWallCoyoteFrames = 6;         // 壁コヨーテ（離床後も受付）

	// 壁スライド（Hollow Knight 風）
	static inline const float kWallSlideMaxFallSpeed = 0.12f; // 壁方向入力中の最大落下速度

	static inline const Vector3 kattackVelocity_{0.4f, 0.0f, 0.0f};

private: // ===== データメンバ =====
	std::unique_ptr<IPlayerState> state_{};

	// 物理
	Vector3 velocity_{}; // 速度（フレーム単位）
	bool onGround_ = true; // 接地中か
	LRDirection lrDirection_ = LRDirection::kRight; // 向き
	float turnFirstRotationY_ = 0.0f; // 旋回開始角
	float turnTimer_ = 0.0f; // 旋回残り時間

	// 入力補助
	int coyoteCounter_ = 0; // コヨーテタイムカウンタ
	int jumpBufferCounter_ = 0; // ジャンプバッファカウンタ

	// 二段ジャンプ管理
	int  airJumpsLeft_ = kMaxAirJumps;
	bool jumpHeldPrev_ = false;

	// 壁ジャン状態管理
	bool isTouchingWall_ = false; // 今フレーム壁に触れている（空中）
	int  lastWallDir_ = 0;        // 最後に触れた壁の向き: +1=右, -1=左, 0=なし
	int  wallCoyoteCounter_ = 0;  // 壁コヨーテ残フレーム

	// Transformとワールド行列
	Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	Matrix4x4 worldMatrix_{}; // S*Ry*T（現在はY回転のみ対応）

	// 描画
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