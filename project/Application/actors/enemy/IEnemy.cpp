#define NOMINMAX
#include "IEnemy.h"
#include "actors/player/Player.h"
#include "contents/MapChipField.h"
#include "function/Math.h"
#include <algorithm>
#include <numbers>

void IEnemy::Initialize(const Vector3& position) {
    transform_.translate = position;
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    isDead_ = false;
	velocity_ = {};
	onGround_ = false;
	isTouchingWall_ = false;
}

void IEnemy::OnCollision(Player* player) {
    // プレイヤーが攻撃中でなければ何もしない
    if (!player->IsAttacking()) {
        return;
    }
    // プレイヤーがダッシュ中なら何もしない
    if (player->IsDashing()) {
        return;
    }
    // 死亡処理
    isDead_ = true;
}

AABB IEnemy::GetAABB() const {
    AABB aabb;
    aabb.min = { transform_.translate.x - width_ / 2.0f, transform_.translate.y - height_ / 2.0f, transform_.translate.z - width_ / 2.0f };
    aabb.max = { transform_.translate.x + width_ / 2.0f, transform_.translate.y + height_ / 2.0f, transform_.translate.z + width_ / 2.0f };
    return aabb;
}

// ワールド座標を取得
Vector3 IEnemy::GetWorldPosition() const {

    // ワールド座標を入れる変数
    Vector3 worldPos;
    // ワールド行列の平行移動成分を取得(ワールド座標)
    worldPos.x = worldMatrix_.m[3][0];
    worldPos.y = worldMatrix_.m[3][1];
    worldPos.z = worldMatrix_.m[3][2];

    return worldPos;
}

void IEnemy::BehaviorMoveUpdate() {
	// 接地している場合のみ行動
	if (onGround_) {
		bool shouldTurn = false;

		// --- 崖チェック ---
		Vector3 footPosition = transform_.translate;
		float checkOffsetX = (lrDirection_ == LRDirection::kRight) ? width_ / 2.0f : -width_ / 2.0f;
		footPosition.x += checkOffsetX;
		footPosition.y -= height_ / 2.0f + 0.1f; // 少し下をチェック

		MapChipField::IndexSet footIndex = mapChipField_->GetMapChipIndexSetByPosition(footPosition);
		if (mapChipField_->GetMapChipTypeByIndex(footIndex.xIndex, footIndex.yIndex) == MapChipType::kBlank) {
			shouldTurn = true;
		}

		// --- 壁チェック ---
		if (isTouchingWall_) {
			shouldTurn = true;
		}

		// --- 方向転換処理 ---
		if (shouldTurn) {
			if (lrDirection_ == LRDirection::kLeft) {
				lrDirection_ = LRDirection::kRight;
				transform_.rotate.y = std::numbers::pi_v<float> / 2.0f;
			}
			else {
				lrDirection_ = LRDirection::kLeft;
				transform_.rotate.y = -std::numbers::pi_v<float> / 2.0f;
			}
		}

		// --- 速度設定 ---
		float moveSpeed = (lrDirection_ == LRDirection::kLeft) ? -kDefaultMoveSpeed : kDefaultMoveSpeed;
		velocity_.x = moveSpeed;
	}

	ApplyGravity();

	CollisionMapInfo info{};
	info.amountMove = velocity_;
	CollisionDetection(info);

	MoveAccordingly(info);
	ContactGround(info);
	ContactWall(info);
}

void IEnemy::ApplyGravity() {
	if (!onGround_) {
		velocity_ = Math::Add(velocity_, Vector3(0.0f, -kgravityAcceleration, 0.0f));
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

void IEnemy::CollisionDetection(CollisionMapInfo& info) {
	if (!mapChipField_) {
		return;
	}
	Vector3 base = transform_.translate;

	float dy = ResolveVerticalFrom(base, info.amountMove.y, info);
	base.y += dy;

	float dx = ResolveHorizontalFrom(base, info.amountMove.x, info);

	info.amountMove = Vector3{ dx, dy, 0.0f };
}

void IEnemy::MoveAccordingly(const CollisionMapInfo& info) {
	transform_.translate = Math::Add(transform_.translate, info.amountMove);
}

void IEnemy::ContactGround(const CollisionMapInfo& info) {
	if (info.isContactGround) {
		if (velocity_.y <= 0.0f) {
			onGround_ = true;
			velocity_.y = 0.0f;
		}
	}
	else {
		onGround_ = false;
	}
}

void IEnemy::ContactWall(const CollisionMapInfo& info) {
	isTouchingWall_ = info.isContactWall;
	if (info.isContactWall) {
		// 壁に衝突したら水平速度を0にする
		velocity_.x = 0.0f;
	}
}

bool IEnemy::IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const {
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(p);
	if (outIdx) { *outIdx = idx; }
	MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
	if (t == MapChipType::kBlock) {
		MapChipField::Rect r = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		if (outRect) { *outRect = r; }
		return true;
	}
	return false;
}

float IEnemy::ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const {
	if (dy == 0.0f) { return 0.0f; }

	const float hx = width_ * 0.5f;
	const float hy = height_ * 0.5f;

	float allowed = dy;
	if (dy > 0.0f) {
		const float topNew = base.y + dy + hy;
		Vector3 pL{ base.x - hx, topNew, 0.0f };
		Vector3 pR{ base.x + hx, topNew, 0.0f };

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pL, &idx, &r) || IsSolidAt(pR, &idx, &r)) {
			float cand = (r.bottom - kMBlank) - (base.y + hy);
			allowed = std::min(allowed, cand);
			info.isContactCeiling = true;
		}
	}
	else {
		const float botNew = base.y + dy - hy;
		Vector3 pL{ base.x - hx, botNew, 0.0f };
		Vector3 pR{ base.x + hx, botNew, 0.0f };

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pL, &idx, &r) || IsSolidAt(pR, &idx, &r)) {
			float cand = (r.top + kMBlank) - (base.y - hy);
			allowed = std::max(allowed, cand);
			info.isContactGround = true;
		}
	}
	return allowed;
}

float IEnemy::ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const {
	if (dx == 0.0f) { return 0.0f; }

	const float hx = width_ * 0.5f;
	const float hy = height_ * 0.5f;

	float allowed = dx;
	if (dx > 0.0f) {
		const float rightNew = base.x + dx + hx;
		Vector3 pT{ rightNew, base.y + hy * 0.8f, 0.0f }; // サンプリングポイントを少し下げる
		Vector3 pB{ rightNew, base.y - hy * 0.8f, 0.0f }; // サンプリングポイントを少し上げる

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pT, &idx, &r) || IsSolidAt(pB, &idx, &r)) {
			float cand = (r.left - kMBlank) - (base.x + hx);
			allowed = std::min(allowed, cand);
			info.isContactWall = true;
			info.wallDir = +1;
		}
	}
	else {
		const float leftNew = base.x + dx - hx;
		Vector3 pT{ leftNew, base.y + hy * 0.8f, 0.0f }; // サンプリングポイントを少し下げる
		Vector3 pB{ leftNew, base.y - hy * 0.8f, 0.0f }; // サンプリングポイントを少し上げる

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pT, &idx, &r) || IsSolidAt(pB, &idx, &r)) {
			float cand = (r.right + kMBlank) - (base.x - hx);
			allowed = std::max(allowed, cand);
			info.isContactWall = true;
			info.wallDir = -1;
		}
	}
	return allowed;
}

void IEnemy::UpdateMatrix() {
	worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
}