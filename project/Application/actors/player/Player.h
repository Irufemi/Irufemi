#pragma once

#include "Irufemi.h"
#include "PlayerState.h"
#include "PlayerPhysics.h" // 追加
#include "Core/Type/LRDirection.h"
#include "contents/mapChipField/MapChipField.h"
#include <cstdint>
#include <memory>

// 前方宣言
class IEnemy;
class Camera;
class InputManager;
struct IPlayerState;
struct PlayerStateRoot;
struct PlayerStateDash;

/// @class Player
/// @brief プレイヤーキャラクターを管理するクラス
/// @details プレイヤーの入力、状態遷移、物理挙動、描画、他オブジェクトとの衝突など、
///          プレイヤーに関連するすべての要素を統括します。
class Player {
public:

	// --- ライフサイクル ---
	/// @brief プレイヤーの初期化
	/// @param model モデル
	/// @param camera カメラ
	/// @param inputManager 入力マネージャー
	/// @param position 初期位置
	void Initialize(ObjClass* model, Camera* camera, InputManager* inputManager, Vector3& position);
	/// @brief 毎フレーム更新
	void Update();
	/// @brief 描画
	void Draw();

	// --- ゲーム連携 ---
	/// @brief 敵との衝突時に呼ばれる
	/// @param enemy 衝突した敵
	void OnCollision(const IEnemy* enemy);
	/// @brief マップチップフィールドを設定
	/// @param mapChipField マップチップフィールド
	void SetMapChipField(MapChipField* mapChipField);
	/// @brief ダメージを受ける
	/// @param damage 受けるダメージ量
	/// @param enemyPosition 敵の位置
	void TakeDamage(const int& damage, const Vector3& enemyPosition);

	// --- 状態取得(読み取り専用) ---
	/// @brief 現在の速度を取得
	/// @return 速度
	const Vector3& GetVelocity() const;
	/// @brief 現在の座標を取得
	/// @return 座標
	const Vector3& GetTranslate() const { return transform_.translate; }
	/// @brief ワールド座標を取得
	/// @return ワールド座標
	Vector3 GetWorldPosition()const;
	/// @brief AABB(当たり判定)を取得
	/// @return AABB
	AABB GetAABB()const;
	/// @brief 攻撃用のAABBを取得
	/// @return 攻撃用AABB
	AABB GetAttackAABB()const; // 攻撃用AABBを取得する関数を追加
	/// @brief 現在の向きを取得
	/// @return 向き (LRDirection)
	LRDirection GetLR() const { return lrDirection_; }
	/// @brief 死亡しているか
	/// @return true: 死亡, false: 生存
	bool IsDead() const { return isDead_; }
	/// @brief ダッシュ中か
	/// @return true: ダッシュ中
	bool IsDashing() const;
	/// @brief 攻撃中か
	/// @return true: 攻撃中
	bool IsAttacking() const; // 攻撃中か判定する関数を追加
	/// @brief ダメージを受けた瞬間か
	/// @return true: ダメージを受けた直後
	bool IsJustDamaged() const { return isJustDamaged_; } // ダメージを受けた瞬間か
	/// @brief 現在のステート名を取得
	/// @return ステート名
	const char* GetStateName() const { return state_ ? state_->Name() : "<none>"; }
	/// @brief ワールド行列を取得
	/// @return ワールド行列
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	/// @brief Transformを取得
	/// @return Transform
	const Transform& GetTransform() const { return transform_; }
	/// @brief 現在のHPを取得
	/// @return HP
	int GetHP() const { return hp_; }
	/// @brief 最大HPを取得
	/// @return 最大HP
	int GetMaxHP() const { return kMaxHP; }

	// セッター
	/// @brief 攻撃エフェクトのモデルを設定
	/// @param obj モデルのポインタ
	void SetAttackEffectModel(ObjClass* obj) { attackEffectModel_ = obj; }
	/// @brief 向きを設定
	/// @param dir 設定する向き
	void SetLRDirection(const LRDirection& dir) { lrDirection_ = dir; }

	// --- ステート制御 ---
	/// @brief ステートを変更する
	/// @param next 次のステート
	void ChangeState(std::unique_ptr<IPlayerState> next);

public: // ===== 定数 (PlayerPhysicsからも参照される) =====
	// --- チューニング用パラメータ ---
	static inline const float kAcceleration = 0.018f;        // 地上: 横加速度(横移動では未使用)
	static inline const float kAttenuation = 0.10f;          // 地上: 無入力減衰(横移動では未使用)
	static inline const float kLimitRunSpeed = 0.20f;        // 地上/空中: 横最高速(=入力一定速度)
	static inline const float kAirAcceleration = 0.011f;     // 空中: 横加速度(横移動では未使用)
	static inline const float kAirAttenuation = 0.02f;       // 空中: 無入力減衰(横移動では未使用)
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
	static inline const int   kCoyoteFrames = 6;             // コヨーテタイム(フレーム)
	static inline const int   kJumpBufferFrames = 6;         // ジャンプバッファ(フレーム)
	static inline const int   kMaxAirJumps = 1;              // 2段ジャンプ回数

	// 壁ジャンプ
	static inline const float kWallJumpHorizontal = 0.26f;   // 壁から離れる水平速度
	static inline const float kWallJumpVertical = 0.28f;     // 壁ジャンの上向き速度
	static inline const int   kWallCoyoteFrames = 6;         // 壁コヨーテ
	// 壁スライド(Hollow Knight 風)
	static inline const float kWallSlideMaxFallSpeed = 0.12f;// 壁方向入力中の最大落下速度

	// ダメージ表現
	static inline const float kKnockbackHorizontal = 0.18f; // ダメージ時の水平ノックバック速度
	static inline const float kKnockbackVertical = 0.15f;   // ダメージ時の垂直ノックバック速度

	// 入力一定速度化のスナップ設定
	static inline const float kTimeToFullRun = 0.06f;        // 最高速へ到達する時間[s]
	static inline const float kWallJumpHorizLockTime = 0.10f;// 壁ジャン直後の横入力ロック時間[s]

	static inline const Vector3 kDashVelocity_{ 0.4f, 0.0f, 0.0f };

	// HP
	static inline const int kMaxHP = 200;

	// 無敵時間
	static inline const float kInvincibilityDuration = 1.5f; // 無敵時間[s]

private: // ===== データメンバ =====
	std::unique_ptr<IPlayerState> state_{};
	std::unique_ptr<PlayerPhysics> physics_{}; // 物理コンポーネント

	// 状態
	LRDirection lrDirection_ = LRDirection::kRight;

	// Transformとワールド行列
	Transform transform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	Matrix4x4 worldMatrix_{}; // S*Ry*T(現在はY回転のみ対応)
	Transform dashEffectTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	Matrix4x4 dashEffectWorldMatrix_{}; // S*Ry*T(現在はY回転のみ対応)
	Transform attackEffectTransform_{ {1.0f,1.0f,1.0f}, {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
	Matrix4x4 attackEffectWorldMatrix_{}; // S*Ry*T(現在はY回転のみ対応)

	// 描画
	ObjClass* model_ = nullptr;
	ObjClass* attackEffectModel_ = nullptr;

	Camera* camera_ = nullptr;
	InputManager* inputManager_ = nullptr;

	// マップ
	MapChipField* mapChipField_ = nullptr;

	// 生存
	bool isDead_ = false;

	// HP
	int hp_ = kMaxHP;

	// 無敵時間タイマー
	float invincibilityTimer_ = 0.0f;

	// ダメージフラグ
	bool isJustDamaged_ = false;

	// se (ダッシュ)
	std::unique_ptr<Se> se_dash_ = nullptr;

	// se (攻撃)
	std::unique_ptr<Se> se_slash_ = nullptr;

private: // ===== 内部処理 =====
	void UpdateMatrix();

	// ステートから private へアクセスを許可
	friend struct IPlayerState;
	friend struct PlayerStateRoot;
	friend struct PlayerStateDash;
	friend struct PlayerStateAttack;
	friend class PlayerPhysics; // PlayerPhysicsからのアクセスを許可
};