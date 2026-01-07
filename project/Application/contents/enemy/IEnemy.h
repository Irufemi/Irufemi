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

    // 初期化
    virtual void Initialize(const Vector3& position);
    // 更新
    virtual void Update() = 0;
    // 描画
    virtual void Draw() = 0;

    // 衝突時の処理
    virtual void OnCollision(Player* player);

    // マップチップフィールドをセット
    void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

public: // アクセサ
    // AABBを取得する
    AABB GetAABB() const;
    // 生存フラグを取得する
    bool IsDead() const { return isDead_; }
    // 向きを取得する
    LRDirection GetLRDirection() const { return lrDirection_; }
    // ダメージを取得する
    int GetDamage() const { return damage_; }
    // ワールド座標を取得
    Vector3 GetWorldPosition() const;

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

protected: // メンバ変数
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

