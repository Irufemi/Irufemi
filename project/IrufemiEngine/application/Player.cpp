#define NOMINMAX
#include "Player.h"

#include "function/Ease.h"
#include "MapChipField.h"
#include "function/Math.h"
#include "engine/Input/InputManager.h"
#include <imgui.h>
#include "PlayerState.h"
#include <algorithm>
#include <cassert>
#include <numbers>

// ===== ライフサイクル =====
void Player::Initialize(ObjClass* model, Camera* camera,InputManager *inputManager, Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;
	inputManager_ = inputManager;
	model_->SetPosition(position);
	model_->SetRotate(Vector3{0.0f,std::numbers::pi_v<float> / 2.0f ,0.0f});

	// 初期状態は Root
	ChangeState(MakeRootState());
}

void Player::Update() {
#ifdef _DEBUG
	ImGui::Begin("Player");
	ImGui::Text("State : %s", GetStateName());
	ImGui::Text("OnGround : %s", onGround_ ? "true" : "false");
	ImGui::End();
#endif
	// ステート更新（ここで velocity_ などが変化）
	if (state_) {
		state_->Update(*this);
	}

	// 共通の移動・衝突
	BehaviorMoveUpdate();
}

void Player::Draw() {
	
	model_->Draw();

}

// ===== ステート制御 =====
void Player::ChangeState(std::unique_ptr<IPlayerState> next) {
	if (state_) {
		state_->Exit(*this);
	}
	state_ = std::move(next);
	if (state_) {
		state_->Enter(*this);
	}
}

// ===== 入力・移動 =====
/*
 * MoveInput
 * 目的: 入力を読み取り、横移動・ジャンプ・重力を更新する。
 * 方針: 横は地上/空中とも常時受付。ジャンプは地上またはコヨーテ時のみ。
 * 追加: ジャンプ短押しカット / ジャンプバッファ / 下降時重力強化 に対応。
 */
void Player::MoveInput() {
	// 入力
	const bool right = inputManager_->IsKeyDown('D');
	const bool left = inputManager_->IsKeyDown('A');
	const bool jumpPressed = inputManager_->IsKeyDown('W');

	// ジャンプバッファ（少し前に押していたら保存）
	if (jumpPressed)
		jumpBufferCounter_ = kJumpBufferFrames;
	else if (jumpBufferCounter_ > 0)
		--jumpBufferCounter_;

	// 媒体（地上/空中）で係数を切替
	const float accel = onGround_ ? kAcceleration : kAirAcceleration;
	const float atten = onGround_ ? kAttenuation : kAirAttenuation;

	// 横移動（常時）
	if (right || left) {
		Vector3 a{};
		if (right) {
			if (velocity_.x < 0.0f) {
				velocity_.x *= (1.0f - atten);
			} // 逆向き慣性を少し殺す
			a.x += accel;
			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = model_->GetRotate().y;
				turnTimer_ = kTimeTurn;
			}
		} else { // left
			if (velocity_.x > 0.0f) {
				velocity_.x *= (1.0f - atten);
			}
			a.x -= accel;
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = model_->GetRotate().y;
				turnTimer_ = kTimeTurn;
			}
		}
		velocity_ = Math::Add(velocity_, a);
		velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
	} else {
		velocity_.x *= (1.0f - atten); // 入力なし→その媒体の減衰で自然に減速
	}

	// ジャンプ（地上またはコヨーテ中のみ）
	if ((onGround_ || coyoteCounter_ > 0) && jumpPressed) {
		velocity_.y = kJumpAcceleration; // ★ 加算ではなく代入：高さを安定させる
		onGround_ = false;
		coyoteCounter_ = 0;
		jumpBufferCounter_ = 0; // 消費
	}

	// ジャンプカット（上昇中にボタンを離したら上向きを削る）
	const bool jumpHeld = jumpPressed; // 単純なポーリングAPI想定
	if (!jumpHeld && velocity_.y > 0.0f) {
		velocity_.y *= kJumpCutFactor;
	}

	// 重力（空中のみ）— 下降時は少し強め
	if (!onGround_) {
		float g = (velocity_.y <= 0.0f) ? kgravityAcceleration * kFallGravityScale : kgravityAcceleration;
		velocity_ = Math::Add(velocity_, Vector3(0.0f, -g, 0.0f));
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

/*
 * BehaviorMoveUpdate
 * 目的: 速度に基づく移動を、マップチップとの衝突でクリップしつつ反映する。
 * 手順: 1) 旋回演出 2) 衝突解決（Y→X） 3) 接触後処理 4) 行列更新
 */
void Player::BehaviorMoveUpdate() {
	// 1) 見た目の向き補間
	TurningControl();

	// 2) 軸分離で衝突解決
	CollisionMapInfo info{};
	info.amountMove = velocity_;
	CollisionDetection(info);

	// 3) 反映と接触後処理
	MoveAccordingly(info);
	ContactCeiling(info);
	ContactGround(info);
	ContactWall(info);

	// 4) 行列更新
	UpdateMatrix();

	model_->Update();
}

// ===== マップ衝突 =====
/*
 * CollisionDetection
 * 目的: 移動量を Y→X の順でクリップし、接触フラグ（天井/地面/壁）を立てる。
 * 対応: (旧) Top/Bottom/Right/Left の4関数を統合。
 */
void Player::CollisionDetection(CollisionMapInfo& info) {
	Vector3 base = model_->GetPosition();

	// 1) Y（天井/地面）
	float dy = ResolveVerticalFrom(base, info.amountMove.y, info);
	base.y += dy;

	// 2) X（壁）— Y を反映した基準位置で判定
	float dx = ResolveHorizontalFrom(base, info.amountMove.x, info);

	info.amountMove = Vector3{dx, dy, 0.0f};
}

/*
 * ResolveVerticalFrom
 * 目的: Y 移動 dy を、頭/足の左右2点サンプルで検出したブロックに対してクリップする。
 * 方法: 上昇(頭): rect.bottom と自機 top の距離で dy を短縮（-kMBlank）。
 * 下降(足): rect.top と自機 bottom の距離で dy を伸長（+kMBlank）。
 * 対応: (旧) MapCollisionTop / MapCollisionBottom の統合。
 */
float Player::ResolveVerticalFrom(const Vector3& base, float dy, CollisionMapInfo& info) const {
	if (dy == 0.0f)
		return 0.0f;

	const float hx = kWidth * 0.5f;
	const float hy = kHeight * 0.5f;

	float allowed = dy;
	if (dy > 0.0f) {
		// 上昇: 新しい Y 位置で左右の「頭の点」を調べる
		const float topNew = base.y + dy + hy;
		Vector3 pL{base.x - hx, topNew, 0.0f};
		Vector3 pR{base.x + hx, topNew, 0.0f};

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pL, &idx, &r)) {
			float cand = (r.bottom - kMBlank) - (base.y + hy);
			allowed = std::min(allowed, cand);
			info.isContactCeiling = true;
		}
		if (IsSolidAt(pR, &idx, &r)) {
			float cand = (r.bottom - kMBlank) - (base.y + hy);
			allowed = std::min(allowed, cand);
			info.isContactCeiling = true;
		}
	} else {
		// 下降: 新しい Y 位置で左右の「足の点」を調べる
		const float botNew = base.y + dy - hy;
		Vector3 pL{base.x - hx, botNew, 0.0f};
		Vector3 pR{base.x + hx, botNew, 0.0f};

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pL, &idx, &r)) {
			float cand = (r.top + kMBlank) - (base.y - hy);
			allowed = std::max(allowed, cand);
			info.isContactGround = true;
		}
		if (IsSolidAt(pR, &idx, &r)) {
			float cand = (r.top + kMBlank) - (base.y - hy);
			allowed = std::max(allowed, cand);
			info.isContactGround = true;
		}
	}
	return allowed;
}

/*
 * ResolveHorizontalFrom
 * 目的: X 移動 dx を、左右端の上下2点サンプルで検出したブロックに対してクリップする。
 * 方法: 右移動: rect.left と自機 right の距離で dx を短縮（-kMBlank）。
 * 左移動: rect.right と自機 left の距離で dx を伸長（+kMBlank）。
 * 対応: (旧) MapCollisionRight / MapCollisionLeft の統合。
 */
float Player::ResolveHorizontalFrom(const Vector3& base, float dx, CollisionMapInfo& info) const {
	if (dx == 0.0f)
		return 0.0f;

	const float hx = kWidth * 0.5f;
	const float hy = kHeight * 0.5f;

	float allowed = dx;
	if (dx > 0.0f) {
		// 右移動: 新しい X 位置で上下の「右端の点」を調べる
		const float rightNew = base.x + dx + hx;
		Vector3 pT{rightNew, base.y + hy, 0.0f};
		Vector3 pB{rightNew, base.y - hy, 0.0f};

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pT, &idx, &r)) {
			float cand = (r.left - kMBlank) - (base.x + hx);
			allowed = std::min(allowed, cand);
			info.isContactWall = true;
		}
		if (IsSolidAt(pB, &idx, &r)) {
			float cand = (r.left - kMBlank) - (base.x + hx);
			allowed = std::min(allowed, cand);
			info.isContactWall = true;
		}
	} else {
		// 左移動: 新しい X 位置で上下の「左端の点」を調べる
		const float leftNew = base.x + dx - hx;
		Vector3 pT{leftNew, base.y + hy, 0.0f};
		Vector3 pB{leftNew, base.y - hy, 0.0f};

		MapChipField::IndexSet idx;
		MapChipField::Rect r;
		if (IsSolidAt(pT, &idx, &r)) {
			float cand = (r.right + kMBlank) - (base.x - hx);
			allowed = std::max(allowed, cand);
			info.isContactWall = true;
		}
		if (IsSolidAt(pB, &idx, &r)) {
			float cand = (r.right + kMBlank) - (base.x - hx);
			allowed = std::max(allowed, cand);
			info.isContactWall = true;
		}
	}
	return allowed;
}

void Player::MoveAccordingly(const CollisionMapInfo& info) {
	model_->SetPosition(Math::Add(model_->GetPosition(), info.amountMove));
}

/*
 * IsSolidAt
 * 目的: 座標 p の属するタイルがブロックか判定し、必要ならその Index/Rect を返す。
 * 注意: 範囲外は MapChipField 側で kBlank を返す想定。
 */
bool Player::IsSolidAt(const Vector3& p, MapChipField::IndexSet* outIdx, MapChipField::Rect* outRect) const {
	auto idx = mapChipField_->GetMapChipIndexSetByPosition(p);
	if (outIdx)
		*outIdx = idx;
	MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
	if (t == MapChipType::kBlock) {
		if (outRect)
			*outRect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
		return true;
	}
	return false;
}

// ===== 接触後の後処理 =====
void Player::ContactCeiling(const CollisionMapInfo& info) {
	if (info.isContactCeiling && velocity_.y > 0.0f) {
		velocity_.y = 0.0f;
	}
}

void Player::ContactGround(const CollisionMapInfo& info) {
	// 空中→接地に遷移したフレーム？
	if (!onGround_ && info.isContactGround) {
		// 着地直前に押していたら、着地フレームで即ジャンプ（バッファ）
		if (jumpBufferCounter_ > 0) {
			velocity_.y = kJumpAcceleration;
			onGround_ = false;
			jumpBufferCounter_ = 0;
			coyoteCounter_ = 0; // バッファジャンプ直後はコヨーテを無効化
			return;             // このフレームはここで確定（下のコヨーテ付与に入らない）
		}
		// ただの着地
		onGround_ = true;
		velocity_.x *= (1.0f - kAttenuationLanding);
	} 
	// 2) 接地中だったが、このフレームは地面に触れていない＝足場から離れた
	else if (onGround_ && !info.isContactGround) {
		onGround_ = false; // ★ ここが今回のキモ：離床時に必ず空中へ
	}

	 // 3) コヨーテ更新：最終的に接地している時だけ付与
	if (info.isContactGround && onGround_) {
		coyoteCounter_ = kCoyoteFrames;
	} else if (coyoteCounter_ > 0) {
		--coyoteCounter_;
	}
}

void Player::ContactWall(const CollisionMapInfo& info) {
	if (info.isContactWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// ===== 見た目の向き制御 =====
void Player::TurningControl() {
	if (turnTimer_ <= 0.0f) {
		model_->SetRotateY((lrDirection_ == LRDirection::kRight) ? std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f) ;
		return;
	}
	// 簡易な固定Δt補間（60fps想定）
	const float dt = 1.0f / 60.0f;
	float t = std::clamp(1.0f - (turnTimer_ / kTimeTurn), 0.0f, 1.0f);
	float target = (lrDirection_ == LRDirection::kRight) ? std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;
	model_->SetRotateY(Lerp(turnFirstRotationY_, target, EaseOutSine(t)));
	turnTimer_ = std::max(0.0f, turnTimer_ - dt);
}

void Player::UpdateMatrix() {


}

// ===== 幾何ユーティリティ =====
Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	const float hx = kWidth * 0.5f;
	const float hy = kHeight * 0.5f;
	switch (corner) {
	case kRightBottom:
		return Math::Add(center, Vector3{+hx, -hy, 0.0f});
	case kLeftBottom:
		return Math::Add(center, Vector3{-hx, -hy, 0.0f});
	case kRightTop:
		return Math::Add(center, Vector3{+hx, +hy, 0.0f});
	case kLeftTop:
		return Math::Add(center, Vector3{-hx, +hy, 0.0f});
	default:
		return center;
	}
}

// ===== 旧個別判定（参考用・未使用） =====
void Player::MapCollisionTop(CollisionMapInfo& info) { (void)info; /* legacy */ }
void Player::MapCollisionBottom(CollisionMapInfo& info) { (void)info; /* legacy */ }
void Player::MapCollisionRight(CollisionMapInfo& info) { (void)info; /* legacy */ }
void Player::MapCollisionLeft(CollisionMapInfo& info) { (void)info; /* legacy */ }

// ===== OnCollision =====
void Player::OnCollision(const Enemy* enemy) {
	(void)enemy;
	if (IsAttack()) {
		return;
	}
	isDead_ = true;
}

// ===== 補助 =====
bool Player::IsAttack() const { return state_ && state_->IsAttack(); }

// ===== 位置・AABB =====
/*
 * GetWorldPosition
 * 目的: プレイヤの“いまの”ワールド座標（中心）を返す。
 * 備考: 行列は BehaviorMoveUpdate() → UpdateMatrix() 内で更新済みなので、
 *       ここでは translation_ をそのまま返せば十分。
 */
Vector3 Player::GetWorldPosition() {

	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得(ワールド座標)
	worldPos.x = model_->GetTransformationMatrix().world.m[3][0];
	worldPos.y = model_->GetTransformationMatrix().world.m[3][1];
	worldPos.z = model_->GetTransformationMatrix().world.m[3][2];

	return worldPos;
}
/*
 * GetAABB
 * 目的: 当たり判定（AABB）を返す。X/Y は kWidth/kHeight から半径を計算。
 * Z は2D運用のため薄めに設定（必要なら調整）。
 */
AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}