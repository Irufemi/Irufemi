#include "NormalEnemy.h"

#include "camera/Camera.h"
#include "actors/player/Player.h"
#include "function/Math.h"
#include "scene/inGame/GameScene.h"
#include "function/Ease.h"
#include "3D/ObjClass.h"
#include "contents/mapChipField/MapChipField.h"
#include <numbers>
#include <cmath>
#include <cassert>
#include <algorithm>

NormalEnemy::NormalEnemy(GameScene* gameScene, Camera* camera)
    : gameScene_(gameScene), camera_(camera) {
    // 基底クラスの当たり判定サイズを設定
    SetWidth(1.0f);
    SetHeight(1.0f);
}

void NormalEnemy::Initialize(const Vector3& position) {
    // 基底クラスの初期化を呼び出す
    IEnemy::Initialize(position);

    GetModel() = std::make_unique<ObjClass>();
    GetModel()->Initialize(camera_, "enemy.obj"); // モデル名を "enemy.obj" に変更

    GetTransform().rotate = { 0.0f, -std::numbers::pi_v<float> / 2.0f, 0.0f }; // 左向きで開始
    SetDirection(LRDirection::kLeft);
    SetVelocity({ -kDefaultMoveSpeed, 0.0f, 0.0f }); // 仮の移動速度

    // ダメージ値を設定
    SetDamage(10);

    // 初期状態は歩行
    behavior_ = Behavior::kWalk;
    BehaviorWalkInitialize();
}

void NormalEnemy::Update() {
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

void NormalEnemy::Draw() {
    if (GetModel()) {
        GetModel()->SetTransform(GetTransform());
        GetModel()->Update();
        GetModel()->Draw();
    }
}

void NormalEnemy::OnCollision(Player* player) {
    // 死亡中、または衝突無効化中は処理しない
    if (behavior_ == Behavior::kDeath || isCollisionDisabled_) {
        return;
    }

    // プレイヤーがダッシュ中の場合は無敵なので処理しない
    if (player->IsDashing()) {
        return;
    }

    // 攻撃が当たったら死亡状態へ移行
    isCollisionDisabled_ = true;
    behaviorRequest_ = Behavior::kDeath;

    // ヒットエフェクトの生成(必要であれば)
    // if (gameScene_) {
    //     // ...
    // }
}

void NormalEnemy::UpdateMatrix() {
    IEnemy::UpdateMatrix();
}

void NormalEnemy::BehaviorWalkInitialize() {
    // 歩行開始時の初期化(必要であれば)
}

void NormalEnemy::BehaviorWalkUpdate() {
	// 基底クラスの移動・衝突解決処理を呼ぶ
	BehaviorMoveUpdate();
}

void NormalEnemy::BehaviorDeathInitialize() {
    deathTimer_ = 0.0f;
    deathStartRotation_ = GetTransform().rotate;
    // プレイヤーの攻撃方向に合わせて吹き飛ぶ回転を設定
    // TODO: Playerのポインタを保持する方法を検討
    deathEndRotation_.x = -std::numbers::pi_v<float> / 2.0f;
}

void NormalEnemy::BehaviorDeathUpdate() {
    const float dt = 1.0f / 60.0f;
    deathTimer_ += dt;

    float t = std::clamp(deathTimer_ / kDeathDuration, 0.0f, 1.0f);

    // Y軸回転
    GetTransform().rotate.y = Lerp(deathStartRotation_.y, deathEndRotation_.y, EaseOutSine(t));

    // X軸回転(演出の後半で倒れる)
    if (t > 0.5f) {
        float fall_t = (t - 0.5f) * 2.0f;
        GetTransform().rotate.x = Lerp(deathStartRotation_.x, deathEndRotation_.x, EaseInSine(fall_t));
    }

    if (deathTimer_ >= kDeathDuration) {
        SetIsDead(true);
    }
}