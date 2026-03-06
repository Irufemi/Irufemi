#include "Player.h"

#include "engine/Input/InputManager.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include "engine/IrufemiEngine.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>

// デストラクタ
Player::~Player() {
}

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine) {
    input_ = input;
    camera_ = camera;
    engine_ = engine;

    // --- モデルの生成と初期化 ---
    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(camera_, "enemy/body.obj");
    obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    // --- 攻撃表示用モデルの生成と初期化 ---
    attackObj_ = std::make_unique<ObjClass>();
    attackObj_->Initialize(camera_, "enemy/body.obj");

    // --- 一人称視点用マスク画像の生成と初期化 ---
    maskSprite_ = std::make_unique<Sprite>();
    maskSprite_->Initialize(camera_, "resources/mask.png");

    // --- ミサイルモデルとデータの初期化（4個分それぞれ用意する） ---
    for (int i = 0; i < kMaxMissiles; ++i) {
        missileObjs_[i] = std::make_unique<ObjClass>();
        missileObjs_[i]->Initialize(camera_, "enemy/body.obj");
        missiles_[i].isActive = false;
    }
    missileCooldown_ = 0;

    // 近接攻撃判定の初期化
    attackCollision_.isActive = false;
    attackCollision_.radius = 2.0f;

    // --- プレイヤーステータスの初期化 ---
    hp_ = 100;
    isDead_ = false;
    invincibleTimer_ = 0;
}

void Player::Update() {
    // ImGui
    obj_->Debug();
    // 死亡している場合は操作や更新を停止する
    if (isDead_) {
        return;
    }

    // 無敵時間タイマーの減算
    if (invincibleTimer_ > 0) {
        invincibleTimer_--;
    }

    // 1. 移動処理
    HandleMovement();

    // 2. 近接攻撃処理（Pキー）
    HandleAttack();

    // 3. ミサイル攻撃処理（Mキー）
    HandleMissile();

    // 4. 視点切り替え(Vキー)
    if (input_->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;
    }

    // 5. カメラをプレイヤーに追従させる
    UpdateCamera();
}

void Player::Draw() {
    // モデルの描画
    if (obj_) {

        // 3Dモデルのトランスフォームを更新
        obj_->SetPosition(translate_);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        // ダメージを受けたあとの無敵時間中は点滅させる（2フレームに1回描画をスキップ）
        bool isBlinking = (invincibleTimer_ > 0 && (invincibleTimer_ % 4) < 2);

        // 一人称視点ではなく、かつ無敵点滅中でなければ描画
        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking && !isDead_) {
            obj_->Draw();
        }
    }

    // 近接攻撃判定が有効な間だけ、分身モデルを描画する
    if (attackObj_ && attackCollision_.isActive && !isDead_) {
        attackObj_->Draw();
    }

    // ミサイルが飛んでいる間だけ、ミサイルごとに個別のモデルを描画する
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive && missileObjs_[i]) {
            missileObjs_[i]->SetPosition(missiles_[i].position);

            // ミサイルを進行方向（速度ベクトル）に向ける計算
            Vector3 mRot = { 0.0f, 0.0f, 0.0f };
            mRot.y = std::atan2(missiles_[i].velocity.x, missiles_[i].velocity.z);
            float xzLen = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x + missiles_[i].velocity.z * missiles_[i].velocity.z);
            mRot.x = std::atan2(-missiles_[i].velocity.y, xzLen);
            missileObjs_[i]->SetRotate(mRot);

            // 少し小さくして描画
            Vector3 missileScale = { scale_.x * 0.4f, scale_.y * 0.4f, scale_.z * 0.4f };
            missileObjs_[i]->SetScale(missileScale);

            // 個別のモデルに対して更新と描画を呼ぶ
            missileObjs_[i]->Update();
            missileObjs_[i]->Draw();
        }
    }

    // --- 一人称視点のとき、画面にマスク画像を被せる ---
    if (viewMode_ == ViewMode::kFirstPerson && !isDead_) {
        if (maskSprite_) {
            maskSprite_->Draw();
        }
    }
}

// ---------------------------------------------------------
// ダメージ処理と判定取得
// ---------------------------------------------------------
PlayerCollider Player::GetCollider() const {
    PlayerCollider col;
    // モデルの足元(translate_)から少し上を判定の中心とする
    col.center = translate_;
    col.center.y += 1.0f;
    col.radius = kColliderRadius;
    return col;
}

void Player::ApplyDamage(int damage) {
    // 既に死亡している、または無敵時間中ならダメージを受けない
    if (isDead_ || invincibleTimer_ > 0) {
        return;
    }

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
        OutputDebugStringA("Player Dead!\n");
    } else {
        // ダメージを受けたら60フレーム（約1秒）無敵になる
        invincibleTimer_ = 60;
        OutputDebugStringA("Player Damaged!\n");
    }
}
// ---------------------------------------------------------


void Player::HandleMovement() {
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    // キー入力取得
    if (input_->IsKeyDown('W')) move.z += 1.0f;
    if (input_->IsKeyDown('S')) move.z -= 1.0f;
    if (input_->IsKeyDown('A')) move.x -= 1.0f;
    if (input_->IsKeyDown('D')) move.x += 1.0f;

    // 平面移動
    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);
        translate_.x += move.x * kMoveSpeed;
        translate_.z += move.z * kMoveSpeed;

        // 三人称視点の時は移動方向を向く
        if (viewMode_ == ViewMode::kThirdPerson) {
            rotate_.y = std::atan2(move.x, move.z);
        }
    }

    // ジャンプと重力
    if (isGrounded_) {
        if (input_->IsKeyPressed(VK_SPACE)) {
            velocity_.y = kJumpForce;
            isGrounded_ = false;
        }
    } else {
        velocity_.y -= kGravity;
        translate_.y += velocity_.y;

        // 地面判定（簡易）
        if (translate_.y <= 0.0f) {
            translate_.y = 0.0f;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}

void Player::HandleAttack() {
    // Pキーで近接攻撃開始
    if (input_->IsKeyPressed('P') && !attackCollision_.isActive) {
        attackCollision_.isActive = true;
        attackActiveTimer_ = 10; // 10フレーム間持続
        OutputDebugStringA("Player Attack Start!\n");
    }

    // 攻撃判定の有効期間中の処理
    if (attackCollision_.isActive) {
        // プレイヤーの向きに合わせて正面に判定を出す
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        // プレイヤーの座標から1.5前方、高さ1.0の位置を中心とする
        attackCollision_.center.x = translate_.x + sinY * 1.5f;
        attackCollision_.center.y = translate_.y + 1.0f;
        attackCollision_.center.z = translate_.z + cosY * 1.5f;

        // 分身モデルを攻撃判定の場所に配置して更新
        if (attackObj_) {
            attackObj_->SetPosition(attackCollision_.center);
            attackObj_->SetRotate(rotate_); // プレイヤーと同じ向き
            attackObj_->SetScale(scale_);   // プレイヤーと同じ大きさ
            attackObj_->Update();
        }

        attackActiveTimer_--;
        if (attackActiveTimer_ <= 0) {
            attackCollision_.isActive = false;
        }
    }
}

void Player::HandleMissile() {
    // クールダウンの処理
    if (missileCooldown_ > 0) {
        missileCooldown_--;
    }

    // Mキーで4個の誘導ミサイルを発射
    if (input_->IsKeyPressed('M') && missileCooldown_ <= 0) {
        missileCooldown_ = 60; // 60フレーム（約1秒）に1回撃てる

        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        // 誘導目標：プレイヤーの20ユニット先
        Vector3 targetPos = {
            translate_.x + sinY * 20.0f,
            translate_.y + 1.0f,
            translate_.z + cosY * 20.0f
        };

        for (int i = 0; i < kMaxMissiles; ++i) {
            missiles_[i].isActive = true;
            missiles_[i].timer = 120; // 120フレーム（約2秒）生存
            missiles_[i].target = targetPos;

            // 初期位置：プレイヤーの少し前
            missiles_[i].position = {
                translate_.x + sinY * 1.0f,
                translate_.y + 1.0f,
                translate_.z + cosY * 1.0f
            };

            // 射出ベクトルを大きくばらけさせる
            float spreadX = ((std::rand() % 100) / 25.0f) - 2.0f; // -2.0 ~ 2.0
            float spreadY = ((std::rand() % 100) / 25.0f) - 0.5f; // -0.5 ~ 3.5 (上方向へ散らす)
            float spreadZ = ((std::rand() % 100) / 25.0f) - 2.0f; // -2.0 ~ 2.0

            // プレイヤーの前方ベクトルに拡散ベクトルを足して初速にする
            missiles_[i].velocity = {
                (sinY * 0.2f) + (spreadX * 0.4f),
                spreadY * 0.4f,
                (cosY * 0.2f) + (spreadZ * 0.4f)
            };
        }
        OutputDebugStringA("Fire 4 Homing Missiles!\n");
    }

    // ミサイルの移動と誘導処理
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive) {
            // 1. 目標へのベクトルを計算
            Vector3 toTarget = {
                missiles_[i].target.x - missiles_[i].position.x,
                missiles_[i].target.y - missiles_[i].position.y,
                missiles_[i].target.z - missiles_[i].position.z
            };

            // ベクトルの正規化（長さを1にする）
            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
            if (dist > 0.001f) {
                toTarget.x /= dist;
                toTarget.y /= dist;
                toTarget.z /= dist;
            }

            // 2. 誘導処理：徐々に目標方向へベクトルを向ける
            float turnSpeed = 0.08f; // 誘導の強さ（大きいほど急カーブ）
            missiles_[i].velocity.x += toTarget.x * turnSpeed;
            missiles_[i].velocity.y += toTarget.y * turnSpeed;
            missiles_[i].velocity.z += toTarget.z * turnSpeed;

            // 3. 速度制限（最高速度を超えないようにする）
            float currentSpeed = std::sqrt(
                missiles_[i].velocity.x * missiles_[i].velocity.x +
                missiles_[i].velocity.y * missiles_[i].velocity.y +
                missiles_[i].velocity.z * missiles_[i].velocity.z
            );
            if (currentSpeed > kMissileSpeed) {
                missiles_[i].velocity.x = (missiles_[i].velocity.x / currentSpeed) * kMissileSpeed;
                missiles_[i].velocity.y = (missiles_[i].velocity.y / currentSpeed) * kMissileSpeed;
                missiles_[i].velocity.z = (missiles_[i].velocity.z / currentSpeed) * kMissileSpeed;
            }

            // 4. 座標の更新
            missiles_[i].position.x += missiles_[i].velocity.x;
            missiles_[i].position.y += missiles_[i].velocity.y;
            missiles_[i].position.z += missiles_[i].velocity.z;

            // 5. 寿命管理
            missiles_[i].timer--;
            if (missiles_[i].timer <= 0) {
                missiles_[i].isActive = false;
            }
        }
    }
}

void Player::UpdateCamera() {
    if (!camera_) return;

    Vector3 cameraPos;

    // ジャンプ時のカメラの揺れ具合（1.0で完全追従、0.0で固定）
    // 0.2〜0.3くらいにすると「少しだけ動く」自然な表現になります。
    const float kCameraJumpFollowRatio = 0.8f;

    if (viewMode_ == ViewMode::kThirdPerson) {
        // 三人称：後ろから見下ろす
        cameraPos.x = translate_.x;
        // ベースの高さ(1.5f)に、プレイヤーのジャンプ量の30%だけ足す
        cameraPos.y = 1.5f + (translate_.y * kCameraJumpFollowRatio);
        cameraPos.z = translate_.z - 5.0f;
        camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
    } else {
        // 一人称：目線の高さ
        cameraPos.x = translate_.x;
        // ベースの高さ(0.0f)に、プレイヤーのジャンプ量の30%だけ足す
        cameraPos.y = 0.0f + (translate_.y * kCameraJumpFollowRatio);
        cameraPos.z = translate_.z;
        camera_->SetRotate({ -0.2f, 0.0f, 0.0f });
    }

    camera_->SetTranslate(cameraPos);
}