#include "ShieldEnemy.h"

#include "camera/Camera.h"
#include "contents/player/Player.h"
#include "function/Math.h"
#include "scene/inGame/GameScene.h"
#include "function/Ease.h"
#include "3D/ObjClass.h"
#include "contents/MapChipField.h"
#include <numbers>
#include <cmath>
#include <cassert>

ShieldEnemy::ShieldEnemy(GameScene* gameScene, Camera* camera)
    : gameScene_(gameScene), camera_(camera) {
    // 基底クラスの当たり判定サイズを設定
    width_ = 1.2f;
    height_ = 1.0f;
}

void ShieldEnemy::Initialize(const Vector3& position) {
    // 基底クラスの初期化を呼び出す
    IEnemy::Initialize(position);

    model_ = std::make_unique<ObjClass>();
    model_->Initialize(camera_, "shieldEnemy.obj");

    transform_.rotate = { 0.0f, -std::numbers::pi_v<float> / 2.0f, 0.0f }; // 左向きで開始
    lrDirection_ = LRDirection::kLeft;
    velocity_ = { -0.05f, 0.0f, 0.0f }; // 仮の移動速度

    // ダメージ値を設定
    damage_ = 10;

    // 初期状態は歩行
    behavior_ = Behavior::kWalk;
    BehaviorWalkInitialize();
}

void ShieldEnemy::Update() {
    // 振る舞いの遷移
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;
        switch (behavior_) {
        case Behavior::kWalk:
            BehaviorWalkInitialize();
            break;
        case Behavior::kDeath:
            BehaviorDeathInitialize();
            break;
        default:
            break;
        }
        behaviorRequest_ = Behavior::kUnknown;
    }

    // 振る舞いごとの更新
    switch (behavior_) {
    case Behavior::kWalk:
        BehaviorWalkUpdate();
        break;
    case Behavior::kDeath:
        BehaviorDeathUpdate();
        break;
    default:
        break;
    }

    UpdateMatrix();
}

void ShieldEnemy::Draw() {
    if (model_) {
        model_->SetTransform(transform_);
        model_->Update();
        model_->Draw();
    }
}

void ShieldEnemy::OnCollision(Player* player) {
    if (behavior_ == Behavior::kDeath || isCollisionDisabled_) {
        return;
    }

    // プレイヤーがダッシュ中なら何もしない
    if (player->IsDashing()) {
        return;
    }

    // プレイヤーと敵の向きを取得
    LRDirection playerDir = player->GetLR();

    // プレイヤーが右向きで敵が左向き、またはプレイヤーが左向きで敵が右向きの場合、正面からの攻撃とみなす
    bool isFrontAttack = (playerDir == LRDirection::kRight && lrDirection_ == LRDirection::kLeft) ||
                         (playerDir == LRDirection::kLeft && lrDirection_ == LRDirection::kRight);

    if (isFrontAttack) {
        // 正面からの攻撃：ダメージ軽減フラグを立てる（エフェクトや音を鳴らすなどの処理もここ）
        isDamageReduction = true;
        // TODO: ガードエフェクトやSEを再生
    }
    else {
        // 背後からの攻撃：デス状態へ移行
        isCollisionDisabled_ = true;
        behaviorRequest_ = Behavior::kDeath;

        // ヒットエフェクト生成
        if (gameScene_) {
            //Vector3 effectPos = Math::Multiply(0.5f, Math::Add(transform_.translate, player->GetTranslate()));
            //gameScene_->CreateHitEffect(effectPos);
        }
    }
}

void ShieldEnemy::UpdateMatrix() {
    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
}

void ShieldEnemy::BehaviorWalkInitialize() {
    // 歩行開始時の初期化（必要であれば）
}

void ShieldEnemy::BehaviorWalkUpdate() {
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
		// isTouchingWall_ は基底クラスの衝突解決で更新される
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
		float moveSpeed = (lrDirection_ == LRDirection::kLeft) ? -0.05f : 0.05f;
		velocity_.x = moveSpeed;
	}

	// 基底クラスの移動・衝突解決処理を呼ぶ
	BehaviorMoveUpdate();
}

void ShieldEnemy::BehaviorDeathInitialize() {
    deathTimer_ = 0.0f;
    deathStartRotation_ = transform_.rotate;
    // プレイヤーの攻撃方向に合わせて吹き飛ぶ回転を設定
    //if (player_ && player_->GetLR() == LRDirection::kRight) {
    //    deathEndRotation_.y = transform_.rotate.y + std::numbers::pi_v<float> * 2.0f;
    //}
    //else {
    //    deathEndRotation_.y = transform_.rotate.y - std::numbers::pi_v<float> * 2.0f;
    //}
    deathEndRotation_.x = -std::numbers::pi_v<float> / 2.0f;
}

void ShieldEnemy::BehaviorDeathUpdate() {
    const float dt = 1.0f / 60.0f;
    deathTimer_ += dt;

    float t = std::clamp(deathTimer_ / kDeathDuration, 0.0f, 1.0f);

    // Y軸回転
    transform_.rotate.y = Lerp(deathStartRotation_.y, deathEndRotation_.y, EaseOutSine(t));

    // X軸回転（演出の後半で倒れる）
    if (t > 0.5f) {
        float fall_t = (t - 0.5f) * 2.0f;
        transform_.rotate.x = Lerp(deathStartRotation_.x, deathEndRotation_.x, EaseInSine(fall_t));
    }

    if (deathTimer_ >= kDeathDuration) {
        isDead_ = true;
    }
}
