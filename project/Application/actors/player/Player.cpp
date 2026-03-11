#include "Player.h"

#include "camera/Camera.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include "Math.h"
#include "Engine/Platform/Input/Mouse.h"
#include "Renderer/LineInstanced/LineClass.h"

// デストラクタ
Player::~Player() {
}

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine, Mouse* mouse) {
    input_ = input;
    camera_ = camera;
    engine_ = engine;
    mouse_ = mouse;

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

    // --- 機関銃モデルの初期化 ---
    machineGunObjLeft_ = std::make_unique<ObjClass>();
    machineGunObjLeft_->Initialize(camera_, "enemy/body.obj");

    machineGunObjRight_ = std::make_unique<ObjClass>();
    machineGunObjRight_->Initialize(camera_, "enemy/body.obj");

    // --- 機関銃の弾モデルの初期化 ---
    for (int i = 0; i < kMaxBullets; ++i) {
        bulletObjs_[i] = std::make_unique<ObjClass>();
        bulletObjs_[i]->Initialize(camera_, "enemy/body.obj");
        bulletObjs_[i]->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f }); // 弾を黄色にする
        bullets_[i].isActive = false;
    }
    machineGunActiveTimer_ = 0;
    machineGunFireTimer_ = 0;

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

#ifdef USE_IMGUI
    lineOBB_ = std::make_unique<Line3DRegion>();
    lineOBB_->Initialize(camera_);
#endif
}

void Player::Update() {
    // 死亡している場合は操作や更新を停止する
    if (isDead_) {
        return;
    }

    // 無敵時間タイマーの減算
    if (invincibleTimer_ > 0) {
        invincibleTimer_--;
    }

#ifdef USE_IMGUI
    ImGui::Begin("Player Settings");
    ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity_, 0.0f, 100.0f);
    // ドラッグ速度は 0.01f のまま、上限を 1.0f に変更
    ImGui::DragFloat("Sensitivity Multiplier", &mouseSensitivityMultiplier_, 0.01f, 0.0f, 1.0f, "%.4f");
    ImGui::End();
#endif

    // --- マウスによる視点操作 ---
    if (mouse_) {
        Vector2 delta = mouse_->GetDelta();

        // 縦横の感度を統一・計算式を調整
        float sensitivityMult = mouseSensitivity_ * mouseSensitivityMultiplier_ * 0.001f;

        rotate_.y += delta.x * sensitivityMult;
        cameraPitch_ += delta.y * sensitivityMult;

        // 【変更部分】一人称時の上下の振り幅の制限
        if (viewMode_ == ViewMode::kThirdPerson) {
            // 三人称視点の制限（元のまま）
            if (cameraPitch_ > -0.01f) cameraPitch_ = -0.02f; // 下を向く限界
            if (cameraPitch_ < -0.2f) cameraPitch_ = -0.2f;   // 上を向く限界
        } else {
            // 一人称視点の制限
            if (cameraPitch_ > 0.6f) cameraPitch_ = 0.6f;     // 下を向く限界
            if (cameraPitch_ < -1.2f) cameraPitch_ = -1.2f;   // 上を向く限界（-1.1f から -1.2f に少し広げました）
        }
    }

    // 1. 移動処理
    HandleMovement();

    // 2. 近接攻撃処理（マウスの左クリックに変更）
    HandleAttack();

    // 3. ミサイル攻撃処理（Mキー）
    HandleMissile();

    // 4. 機関銃の処理（Fキー）
    HandleMachineGun();

    // 5. 視点切り替え(Vキー)
    if (input_->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;

        // 視点を切り替えた直後、現在のカメラの角度が切り替え先の制限を超えていたら補正する
        if (viewMode_ == ViewMode::kThirdPerson) {
            if (cameraPitch_ > -0.02f) cameraPitch_ = -0.02f;
            if (cameraPitch_ < -0.2f) cameraPitch_ = -0.2f;
        }
    }

    // 6. カメラをプレイヤーに追従させる
    UpdateCamera();

#ifdef USE_IMGUI
    if (input_->IsKeyPressedDIK(0x3B /*DIK_F1*/)) {
        isDebugDrawOBB_ = !isDebugDrawOBB_;
    }

    if (lineOBB_) {
        lineOBB_->ClearInstances();
        if (isDebugDrawOBB_) {
            auto addObbLines = [&](const OBB& obb) {
                Vector3 corners[8];
                for (int i = 0; i < 8; ++i) {
                    Vector3 offset = { 0, 0, 0 };
                    offset = Math::Add(offset, Math::Multiply((i & 1) ? obb.size.x : -obb.size.x, obb.orientations[0]));
                    offset = Math::Add(offset, Math::Multiply((i & 2) ? obb.size.y : -obb.size.y, obb.orientations[1]));
                    offset = Math::Add(offset, Math::Multiply((i & 4) ? obb.size.z : -obb.size.z, obb.orientations[2]));
                    corners[i] = Math::Add(obb.center, offset);
                }
                Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
                lineOBB_->AddInstance(corners[0], corners[1], color);
                lineOBB_->AddInstance(corners[1], corners[3], color);
                lineOBB_->AddInstance(corners[3], corners[2], color);
                lineOBB_->AddInstance(corners[2], corners[0], color);
                lineOBB_->AddInstance(corners[4], corners[5], color);
                lineOBB_->AddInstance(corners[5], corners[7], color);
                lineOBB_->AddInstance(corners[7], corners[6], color);
                lineOBB_->AddInstance(corners[6], corners[4], color);
                lineOBB_->AddInstance(corners[0], corners[4], color);
                lineOBB_->AddInstance(corners[1], corners[5], color);
                lineOBB_->AddInstance(corners[2], corners[6], color);
                lineOBB_->AddInstance(corners[3], corners[7], color);
            };

            // Player OBB
            addObbLines(GetCollider().obb);

            // Attack OBB
            if (attackCollision_.isActive) {
                OBB attackObb;
                attackObb.center = attackCollision_.center;
                attackObb.orientations[0] = { 1.0f, 0.0f, 0.0f };
                attackObb.orientations[1] = { 0.0f, 1.0f, 0.0f };
                attackObb.orientations[2] = { 0.0f, 0.0f, 1.0f };
                attackObb.size = { attackCollision_.radius, attackCollision_.radius, attackCollision_.radius };
                addObbLines(attackObb);
            }

            // Missiles OBB
            for (int i = 0; i < kMaxMissiles; ++i) {
                if (missiles_[i].isActive) {
                    OBB missileObb;
                    missileObb.center = missiles_[i].position;
                    // ミサイルの進行方向から回転行列を作ることもできるが、簡略化のため軸固定
                    missileObb.orientations[0] = { 1.0f, 0.0f, 0.0f };
                    missileObb.orientations[1] = { 0.0f, 1.0f, 0.0f };
                    missileObb.orientations[2] = { 0.0f, 0.0f, 1.0f };
                    missileObb.size = { 1.0f, 1.0f, 1.0f };
                    addObbLines(missileObb);
                }
            }

            // Bullets OBB
            for (int i = 0; i < kMaxBullets; ++i) {
                if (bullets_[i].isActive) {
                    OBB bulletObb;
                    bulletObb.center = bullets_[i].position;
                    bulletObb.orientations[0] = { 1.0f, 0.0f, 0.0f };
                    bulletObb.orientations[1] = { 0.0f, 1.0f, 0.0f };
                    bulletObb.orientations[2] = { 0.0f, 0.0f, 1.0f };
                    bulletObb.size = { 0.5f, 0.5f, 0.5f };
                    addObbLines(bulletObb);
                }
            }
        }
        lineOBB_->Update();
    }
#endif
}

void Player::Draw() {
    // ダメージを受けたあとの無敵時間中は点滅させる（2フレームに1回描画をスキップ）
    bool isBlinking = (invincibleTimer_ > 0 && (invincibleTimer_ % 4) < 2);

    // モデルの描画
    if (obj_) {

        // 3Dモデルのトランスフォームを更新
        obj_->SetPosition(translate_);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        // 一人称視点ではなく、かつ無敵点滅中でなければ描画
        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking && !isDead_) {
            obj_->Draw();
        }
    }

    // 近接攻撃判定が有効な間だけ、分身モデルを描画する
    if (attackObj_ && attackCollision_.isActive && !isDead_) {
        attackObj_->Draw();
    }

    // --- 機関銃（肩）の描画 ---
    if (machineGunObjLeft_ && machineGunObjRight_ && !isDead_) {
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);
        float rightX = cosY;
        float rightZ = -sinY;

        // 肩の位置を計算（プレイヤーの左右）
        Vector3 leftShoulder = { translate_.x - rightX * 0.7f, translate_.y + 1.0f, translate_.z - rightZ * 0.7f };
        Vector3 rightShoulder = { translate_.x + rightX * 0.7f, translate_.y + 1.0f, translate_.z + rightZ * 0.7f };

        // オートエイムのため、銃口をターゲットに向ける
        Vector3 playerCenter = { translate_.x, translate_.y + 1.0f, translate_.z };
        Vector3 aimPos = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z }; // 敵の少し上を狙う
        Vector3 toTarget = { aimPos.x - playerCenter.x, aimPos.y - playerCenter.y, aimPos.z - playerCenter.z };

        Vector3 rot = { 0.0f, 0.0f, 0.0f };
        float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
        if (dist > 0.001f) {
            rot.y = std::atan2(toTarget.x, toTarget.z);
            float xzLen = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
            rot.x = std::atan2(-toTarget.y, xzLen);
        } else {
            rot.y = rotate_.y;
            rot.x = cameraPitch_;
        }

        machineGunObjLeft_->SetPosition(leftShoulder);
        machineGunObjLeft_->SetRotate(rot);
        machineGunObjLeft_->SetScale({ 0.1f, 0.1f, 0.3f }); // 細長い形にする
        machineGunObjLeft_->Update();

        machineGunObjRight_->SetPosition(rightShoulder);
        machineGunObjRight_->SetRotate(rot);
        machineGunObjRight_->SetScale({ 0.1f, 0.1f, 0.3f });
        machineGunObjRight_->Update();

        // 一人称視点ではなく、かつ無敵点滅中でなければ描画
        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking) {
            machineGunObjLeft_->Draw();
            machineGunObjRight_->Draw();
        }
    }

    // --- 機関銃の弾の描画 ---
    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive && bulletObjs_[i] && !isDead_) {
            bulletObjs_[i]->SetPosition(bullets_[i].position);

            // 飛んでいく方向に向ける
            Vector3 bRot = { 0.0f, std::atan2(bullets_[i].velocity.x, bullets_[i].velocity.z), 0.0f };
            float bxzLen = std::sqrt(bullets_[i].velocity.x * bullets_[i].velocity.x + bullets_[i].velocity.z * bullets_[i].velocity.z);
            bRot.x = std::atan2(-bullets_[i].velocity.y, bxzLen);

            bulletObjs_[i]->SetRotate(bRot);
            bulletObjs_[i]->SetScale({ 0.05f, 0.05f, 0.2f }); // 弾を線のように細長く

            bulletObjs_[i]->Update();
            bulletObjs_[i]->Draw();
        }
    }

    // ミサイルが飛んでいる間だけ、ミサイルごとに個別のモデルを描画する
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive && missileObjs_[i]) {
            missileObjs_[i]->SetPosition(missiles_[i].position);

            // ミサイルを進行方向（速度ベクトル）に向ける計算
            Vector3 mRot = { 0.0f, std::atan2(missiles_[i].velocity.x, missiles_[i].velocity.z), 0.0f };
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

#ifdef USE_IMGUI
    if (lineOBB_ && isDebugDrawOBB_ && engine_) {
        engine_->ApplyLineInstancedPSO();
        lineOBB_->Draw();
        engine_->ApplyPSO(); // restore
    }
#endif
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

    // --- OBBの当たり判定データの追加 ---
    col.obb.center = col.center;

    // プレイヤーのY軸回転から回転行列を生成
    Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, rotate_.y));

    // 行列から X軸, Y軸, Z軸 の方向ベクトルを抽出
    col.obb.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
    col.obb.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
    col.obb.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };

    // OBBのサイズ（各軸の半分の長さ）を設定（必要に応じて調整してください）
    col.obb.size = { 0.5f, 1.0f, 0.5f };

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

        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        float moveX = move.x * cosY + move.z * sinY;
        float moveZ = -move.x * sinY + move.z * cosY;

        translate_.x += moveX * kMoveSpeed;
        translate_.z += moveZ * kMoveSpeed;

        // --- フィールド外に出ないための制限 ---
        if (translate_.x > kFieldRangeX)  translate_.x = kFieldRangeX;
        if (translate_.x < -kFieldRangeX) translate_.x = -kFieldRangeX;
        if (translate_.z > kFieldRangeZ)  translate_.z = kFieldRangeZ;
        if (translate_.z < -kFieldRangeZ) translate_.z = -kFieldRangeZ;
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
    // マウスの左クリックで近接攻撃開始
    if (mouse_ && mouse_->IsButtonPressed(Mouse::Button::Left) && !attackCollision_.isActive) {
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
            // モデルの表示サイズは元通りのまま
            attackObj_->SetScale(scale_);
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
        for (int i = 0; i < kMaxMissiles; ++i) {
            missiles_[i].isActive = true;
            missiles_[i].timer = 120; // 120フレーム（約2秒）生存

            missiles_[i].target = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };

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

            missiles_[i].target = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };

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

void Player::HandleMachineGun() {
    // Fキーで機関銃起動
    if (input_->IsKeyPressed('F') && machineGunActiveTimer_ <= 0) {
        machineGunActiveTimer_ = 180; // 3秒間（60FPS想定）撃ち続ける
        machineGunFireTimer_ = 0;
        OutputDebugStringA("MachineGun Start!\n");
    }

    if (machineGunActiveTimer_ > 0) {
        machineGunActiveTimer_--;
        machineGunFireTimer_--;

        // 6フレームに1回（1秒間に10回）両肩から発射
        if (machineGunFireTimer_ <= 0) {
            machineGunFireTimer_ = 6;

            float sinY = std::sin(rotate_.y);
            float cosY = std::cos(rotate_.y);
            float rightX = cosY;
            float rightZ = -sinY;

            // 両肩の位置を計算
            Vector3 leftShoulder = { translate_.x - rightX * 0.7f, translate_.y + 1.0f, translate_.z - rightZ * 0.7f };
            Vector3 rightShoulder = { translate_.x + rightX * 0.7f, translate_.y + 1.0f, translate_.z + rightZ * 0.7f };

            FireMachineGunBullet(leftShoulder);
            FireMachineGunBullet(rightShoulder);
        }
    }

    // 弾の移動と寿命管理
    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive) {
            bullets_[i].position.x += bullets_[i].velocity.x;
            bullets_[i].position.y += bullets_[i].velocity.y;
            bullets_[i].position.z += bullets_[i].velocity.z;

            bullets_[i].timer--;
            if (bullets_[i].timer <= 0) {
                bullets_[i].isActive = false;
            }
        }
    }
}

void Player::FireMachineGunBullet(const Vector3& startPos) {
    for (int i = 0; i < kMaxBullets; ++i) {
        if (!bullets_[i].isActive) {
            bullets_[i].isActive = true;
            bullets_[i].position = startPos;
            bullets_[i].timer = 60; // 1秒で消える（弾速が速いので十分届く）

            Vector3 playerCenter = { translate_.x, translate_.y + 1.0f, translate_.z };
            // オートエイム計算
            Vector3 aimPos = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z }; // 敵の少し上を狙う
            Vector3 toTarget = {
                aimPos.x - playerCenter.x,
                aimPos.y - playerCenter.y,
                aimPos.z - playerCenter.z
            };

            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
            if (dist > 0.001f) {
                toTarget.x /= dist;
                toTarget.y /= dist;
                toTarget.z /= dist;
            } else {
                // 敵がいない場合は正面に撃つ
                float cosPitch = std::cos(cameraPitch_);
                float sinPitch = std::sin(cameraPitch_);
                toTarget = { std::sin(rotate_.y) * cosPitch, -sinPitch, std::cos(rotate_.y) * cosPitch };
            }

            float bulletSpeed = 3.0f; // 弾速（かなり速い）
            bullets_[i].velocity = {
                toTarget.x * bulletSpeed,
                toTarget.y * bulletSpeed,
                toTarget.z * bulletSpeed
            };
            break; // 1発発射したらループを抜ける
        }
    }
}

void Player::UpdateCamera() {
    if (!camera_) return;

    Vector3 cameraPos;
    const float kCameraJumpFollowRatio = 0.8f;

    // --- プレイヤーのどの高さを中心にカメラを回すか（注視点） ---
    // 足元(translate_)から少し上（例：1.5f）をターゲットにする
    Vector3 lookAtTarget = {
        translate_.x,
        translate_.y,
        translate_.z
    };

    if (viewMode_ == ViewMode::kThirdPerson) {
        // --- 下を向くにつれてカメラを近づける処理 ---
        // 割合(0.0 ～ 1.0) を計算。-0.05f(下向き限界)に近づくほど距離を短くする
        float pitchRatio = (cameraPitch_ - (-0.2f)) / (-0.05f - (-0.2f));

        // 範囲外の値を防ぐためのクランプ
        if (pitchRatio < 0.0f) pitchRatio = 0.0f;
        if (pitchRatio > 1.0f) pitchRatio = 1.0f;

        float maxDistance = 5.0f; // 一番上を向いている時の距離（遠い）
        float minDistance = 2.0f; // 一番下を向いている時の距離（近い）

        // 割合に応じて距離を滑らかに変更（線形補間）
        float distance = maxDistance * (1.0f - pitchRatio) + minDistance * pitchRatio;

        float cosPitch = std::cos(cameraPitch_);
        float sinPitch = std::sin(cameraPitch_);
        float cosYaw = std::cos(rotate_.y);
        float sinYaw = std::sin(rotate_.y);

        // 注視点(lookAtTarget)からのオフセットとして計算
        cameraPos.x = lookAtTarget.x - (sinYaw * cosPitch * distance);
        cameraPos.y = lookAtTarget.y - (sinPitch * distance); // 高さも注視点基準
        cameraPos.z = lookAtTarget.z - (cosYaw * cosPitch * distance);

        // 床へのめり込み防止
        if (cameraPos.y < 0.2f) {
            cameraPos.y = 0.2f;
        }

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    } else {
        // 一人称（既存のロジック）
        cameraPos.x = translate_.x;
        cameraPos.y = 1.0f + (translate_.y * kCameraJumpFollowRatio);
        cameraPos.z = translate_.z;

        if (cameraPos.y < 0.2f) cameraPos.y = 0.2f;

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    }
}