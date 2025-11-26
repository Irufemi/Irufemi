#include "Enemy.h"

#include "Ease.h"
#include "Function.h"
#include "GameScene.h"
#include "Math.h"
#include "Player.h"
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// 初期化
void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, Vector3& position,GameScene* gameScene) {

	// NULLポインタチェック
	assert(model);

	// 引数として受け取ったデータをメンバ変数に記録する
	// モデル
	this->model_ = model;
	// カメラ
	this->camera_ = camera;
	// ゲームシーン
	this->gameScene_ = gameScene;

	// ワールド変換の初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = 0.0f;

	// 角度に応じて右向きか左向きか判定する
	 if (worldTransform_.rotation_.y < std::numbers::pi_v<float>) {
		lrDirection_ = LRDirection::kLeft;
	 } else if (worldTransform_.rotation_.y > std::numbers::pi_v<float>) {
		lrDirection_ = LRDirection::kRight;
	 }

	// 速度を設定する
	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};

	walkTimer = 0.0f;
}

// 更新
void Enemy::Update() {

	if (behaviorRequest_ != Behavior::kUnknown) {
		// 振る舞いを変更する
		behavior_ = behaviorRequest_;
		// 各振る舞いごとの初期化を実行
		switch (behavior_) {
		case Behavior::kWalk:
		default:
			// ルートビヘイビアの初期化
			BehaviorWalkInitialize();
			break;
		case Behavior::kDeath:
			// 攻撃ビヘイビアの初期化
			BehaviorDeathInitialize();
			break;
		}
		// 振る舞いリクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;
	}

	// 現在のビヘイビアに応じた処理
	switch (behavior_) {
		// 通常行動
	case Behavior::kWalk:
	default:
		// ルートビヘイビアの更新
		BehaviorWalkUpdate();
		break;
	case Behavior::kDeath:
		// 攻撃ビヘイビアの更新
		BehaviorDeathUpdate();
		break;
	}
}

// 描画
void Enemy::Draw() {

	// 3Dモデルを描画
	model_->Draw(worldTransform_, *camera_);
}

// 歩行行動初期化
void Enemy::BehaviorWalkInitialize() {}

// デス行動初期化
void Enemy::BehaviorDeathInitialize() { 
	deathTimer_ = 0.0f;
	deathStartRotation_ = worldTransform_.rotation_;
	deathEndRotation_.x = -std::numbers::pi_v<float> / 2.0f;
}

// 歩行行動更新
void Enemy::BehaviorWalkUpdate() {

	// 移動
	worldTransform_.translation_ = Add(worldTransform_.translation_, velocity_);

	//// タイマーを加算
	//walkTimer += 1.0f / 60.0f;

	//// 回転アニメーション
	//float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer / kWalkMotionTime);
	//float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	//worldTransform_.rotation_.x = degree / std::numbers::pi_v<float>;

	worldTransform_.rotation_.x += 0.05f;

	// 座標を元に行列の更新を行う
	UpdateWorldTransform(worldTransform_);
}

// デス行動更新
void Enemy::BehaviorDeathUpdate() {

	// アニメーションのタイマーを加算する(1フレーム分の秒数進める)
	deathTimer_ += 1.0f / 60.0f;

	// Y軸まわりの回転角をイージングで変化させる
	worldTransform_.rotation_.y = Lerp(deathStartRotation_.y, deathEndRotation_.y, deathTimer_ / kDeathDuration);

	// X軸まわりの回転角をイージングで変化させる(演出時間の最後4分の1)
	if (deathTimer_ >= kDeathDuration * 3.0f / 4.0f) {
		worldTransform_.rotation_.x = Lerp(deathStartRotation_.x, deathEndRotation_.x, (deathTimer_ - kDeathDuration * 3.0f / 4.0f) / (kDeathDuration / 4.0f));
	}

	// ワールドトランスフォームの行列更新
	UpdateWorldTransform(worldTransform_);

	// アニメーションのタイマーが一定時間に達したらデスフラグを立てる
	if (deathTimer_ >= kDeathDuration) {
		isDead_ = true;
	}
}

void Enemy::OnCollision(const Player* player) {
	(void)player;
	if (behavior_ == Behavior::kDeath) {
		// 敵がやられているなら何もしない
		return;
	}

	// プレイヤーが攻撃中なら敵が死ぬ
	if (player->IsAttack()) {
		// コリジョン無効化フラグを立てる
		isCollisionDisabled_ = true;

		// 敵の振る舞いをデス演出に変更
		behaviorRequest_ = Behavior::kDeath;

		// 敵と自キャラの中間位置にエフェクトを生成
		Vector3 effectPos = Multiply(1.0f/2.0f,Add (worldTransform_.translation_,gameScene_->GetPlayerPosition()));
		gameScene_->CreateHitEffect(effectPos);

		if (player->GetLR() == Player::LRDirection::kRight) {
			deathEndRotation_.y = -std::numbers::pi_v<float> * 2.0f;
		} else if (player->GetLR() == Player::LRDirection::kLeft) {
			deathEndRotation_.y = std::numbers::pi_v<float> * 2.0f;
		}
	}
}

// ワールド座標を取得
Vector3 Enemy::GetWorldPosition() {

	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得(ワールド座標)
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

// AABBを取得
AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	aabb.min_ = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max_ = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// 旋回制御
void Enemy::TurningControl() {

	if (turnTimer_ <= 0.0f) {
		if ((lrDirection_ == LRDirection::kRight && worldTransform_.rotation_.y < std::numbers::pi_v<float>) ||
		    (lrDirection_ == LRDirection::kLeft && worldTransform_.rotation_.y > std::numbers::pi_v<float>)) {
			turnTimer_ = kTimeTurn;
			turnFirstRotationY_ = worldTransform_.rotation_.y = 0.0f;

		} else {
			return;
		}
	} else if (turnTimer_ > 0.0f) {

		// 旋回タイマーを1/60秒だけカウントダウンする
		turnTimer_ -= 1.0f / 60.0f;

		// 左右の自キャラ角度テーブル
		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,       // 右
		    std::numbers::pi_v<float> * 3.0f / 2.0f // 左
		};
		// 状況に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		// 自キャラの角度を設定する
		worldTransform_.rotation_.y = turnTimer_ / kTimeTurn * turnFirstRotationY_ + (1.0f - turnTimer_ / kTimeTurn) * destinationRotationY;
	}
}
