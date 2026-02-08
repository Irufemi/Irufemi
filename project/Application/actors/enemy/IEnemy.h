#pragma once

#include "math/Vector3.h"
#include "math/Transform.h"
#include "math/Matrix4x4.h"
#include "math/shape/AABB.h"
#include "math/LRDirection.h"
#include "contents/MapChipField.h"

#include <memory>

// 前方宣言
class Player;
class ObjClass;

class IEnemy
{
private: // 内部型
	struct CollisionMapInfo {
		bool isContactCeiling = false;
		bool isContactGround = false;
		bool isContactWall = false;
		int  wallDir = 0;
		Vector3 amountMove{};
	};

public: // メンバ関数
    // デストラクタ
    virtual ~IEnemy() = default;

    /// @brief 敵の初期化
    /// @param position 初期座標
    virtual void Initialize(const Vector3& position);
    /// @brief 毎フレーム更新
    virtual void Update() = 0;
    /// @brief 描画
    virtual void Draw() = 0;

    /// @brief プレイヤーとの衝突時に呼ばれる処理
    /// @param player 衝突したプレイヤー
    virtual void OnCollision(Player* player);

    /// @brief マップチップフィールドを設定
    /// @param mapChipField マップチップフィールド
    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

public: // アクセサ
    /// @brief AABB(当たり判定)を取得
    /// @return AABB
	AABB GetAABB() const;
    /// @brief 生存フラグを取得
    /// @return true: 死亡, false: 生存
	bool IsDead() const { return isDead_; }
    /// @brief 向きを取得
    /// @return 向き (LRDirection)
	LRDirection GetLRDirection() const { return lrDirection_; }
    /// @brief プレイヤーに与えるダメージ量を取得
    /// @return ダメージ量
	int GetDamage() const { return damage_; }
    /// @brief ワールド座標を取得
    /// @return ワールド座標
    Vector3 GetWorldPosition() const;

protected: // 派生クラス向けアクセサ
	const Transform& GetTransform() const { return transform_; }
	Transform& GetTransform() { return transform_; }
	const Vector3& GetVelocity() const { return velocity_; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	LRDirection GetDirection() const { return lrDirection_; }
	void SetDirection(const LRDirection& direction) { lrDirection_ = direction; }
	bool IsOnGround() const { return onGround_; }
	bool IsTouchingWall() const { return isTouchingWall_; }
	void SetIsDead(const bool& isDead) { isDead_ = isDead; }
	std::unique_ptr<ObjClass>& GetModel() { return model_; }
	void SetWidth(const float& width) { width_ = width; }
	void SetHeight(const float& height) { height_ = height; }
	void SetDamage(const int& damage) { damage_ = damage; }

protected: // 内部処理
	// 移動と衝突判定
	void BehaviorMoveUpdate();
	// 重力適用
	void ApplyGravity();
	// 衝突検知
	void CollisionDetection(CollisionMapInfo& info);
	// 衝突後の移動
	void MoveAccordingly(const CollisionMapInfo& info);
	// 地面との接触処理
	void ContactGround(const CollisionMapInfo& info);
	// 壁との接触処理
	void ContactWall(const CollisionMapInfo& info);
	// 座標がブロック内か判定
	bool IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const;
	// 垂直方向の衝突解決
	float ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const;
	// 水平方向の衝突解決
	float ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const;
	// 行列の更新
	void UpdateMatrix();

protected: // 定数
	static inline const float kgravityAcceleration = 0.010f;
	static inline const float kLimitFallSpeed = 0.36f;
	static inline const float kMBlank = 0.01f;
	static inline const float kDefaultMoveSpeed = 0.05f;

private: // メンバ変数
    // トランスフォーム
    Transform transform_;
    // ワールド行列
    Matrix4x4 worldMatrix_;
    // モデル
    std::unique_ptr<ObjClass> model_ = nullptr;
    // 幅
    float width_ = 1.0f;
    // 高さ
    float height_ = 1.0f;
    // 生存フラグ
    bool isDead_ = false;
    // 向き
    LRDirection lrDirection_ = LRDirection::kLeft;
    // ダメージ
    int damage_ = 10;
    // マップチップフィールド
    MapChipField* mapChipField_ = nullptr;
	// 速度
	Vector3 velocity_{};
	// 接地フラグ
	bool onGround_ = false;
	// 壁接触フラグ
	bool isTouchingWall_ = false;
};

