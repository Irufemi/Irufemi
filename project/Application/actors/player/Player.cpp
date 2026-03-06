#include "Player.h"

#include "engine/Input/InputManager.h"
#include "camera/Camera.h"
#include "function/Math.h"
#include "engine/IrufemiEngine.h"
#include "engine/Input/Mouse.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>

// ImGui用
#ifdef USE_IMGUI
#include "manager/DebugUI.h" 
#include <imgui.h>
#endif

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

    // ★修正：弾100発ぶんのモデルを個別に作って初期化する
    for (int i = 0; i < kMaxBullets; ++i) {
        bulletObjs_[i] = std::make_unique<ObjClass>();
        bulletObjs_[i]->Initialize(camera_, "enemy/body.obj");
        bulletObjs_[i]->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
        bullets_[i].isActive = false;
    }
    machineGunActiveTimer_ = 0;
    machineGunFireTimer_ = 0;

    // --- ミサイルモデルとデータの初期化 ---
    for (int i = 0; i < kMaxMissiles; ++i) {
        missileObjs_[i] = std::make_unique<ObjClass>();
        missileObjs_[i]->Initialize(camera_, "enemy/body.obj");
        missiles_[i].isActive = false;
    }
    missileCooldown_ = 0;

    attackCollision_.isActive = false;
    attackCollision_.radius = 2.0f;

    hp_ = 100;
    isDead_ = false;
    invincibleTimer_ = 0;
}

void Player::Update() {
    if (isDead_) {
        return;
    }

    if (invincibleTimer_ > 0) {
        invincibleTimer_--;
    }

#ifdef USE_IMGUI
    ImGui::Begin("Player Settings");
    ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity_, 0.0f, 100.0f);
    ImGui::DragFloat("Sensitivity Multiplier", &mouseSensitivityMultiplier_, 0.0001f, 0.0f, 100.0f, "%.4f");
    ImGui::End();
#endif

    // --- マウスによる視点操作 ---
    if (mouse_) {
        Vector2 delta = mouse_->GetDelta();

        float sensitivityMult = mouseSensitivity_ * mouseSensitivityMultiplier_ * 0.0005f;

        rotate_.y += delta.x * sensitivityMult;
        cameraPitch_ += delta.y * sensitivityMult;

        if (cameraPitch_ > 1.4f) cameraPitch_ = 1.4f;
        if (cameraPitch_ < -1.4f) cameraPitch_ = -1.4f;
    }

    // 各アクションの更新
    HandleMovement();
    HandleAttack();
    HandleMissile();
    HandleMachineGun();

    if (input_->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;
    }

    UpdateCamera();
}

void Player::Draw() {
    bool isBlinking = (invincibleTimer_ > 0 && (invincibleTimer_ % 4) < 2);

    if (obj_) {
        obj_->SetPosition(translate_);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking && !isDead_) {
            obj_->Draw();
        }
    }

    if (attackObj_ && attackCollision_.isActive && !isDead_) {
        attackObj_->Draw();
    }

    // --- 機関銃の描画 ---
    if (machineGunObjLeft_ && machineGunObjRight_ && !isDead_) {
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);
        float rightX = cosY;
        float rightZ = -sinY;

        Vector3 leftShoulder = { translate_.x - rightX * 0.7f, translate_.y + 1.0f, translate_.z - rightZ * 0.7f };
        Vector3 rightShoulder = { translate_.x + rightX * 0.7f, translate_.y + 1.0f, translate_.z + rightZ * 0.7f };

        Vector3 playerCenter = { translate_.x, translate_.y + 1.0f, translate_.z };
        Vector3 aimPos = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };
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
        machineGunObjLeft_->SetScale({ 0.1f, 0.1f, 0.3f });
        machineGunObjLeft_->Update();

        machineGunObjRight_->SetPosition(rightShoulder);
        machineGunObjRight_->SetRotate(rot);
        machineGunObjRight_->SetScale({ 0.1f, 0.1f, 0.3f });
        machineGunObjRight_->Update();

        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking) {
            machineGunObjLeft_->Draw();
            machineGunObjRight_->Draw();
        }
    }

    // --- 機関銃の弾の描画 ---
    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive && bulletObjs_[i] && !isDead_) {
            bulletObjs_[i]->SetPosition(bullets_[i].position);

            Vector3 bRot = { 0.0f, std::atan2(bullets_[i].velocity.x, bullets_[i].velocity.z), 0.0f };
            float bxzLen = std::sqrt(bullets_[i].velocity.x * bullets_[i].velocity.x + bullets_[i].velocity.z * bullets_[i].velocity.z);
            bRot.x = std::atan2(-bullets_[i].velocity.y, bxzLen);

            // ★修正：個別のモデルに対して座標や回転を適用する
            bulletObjs_[i]->SetRotate(bRot);
            bulletObjs_[i]->SetScale({ 0.05f, 0.05f, 0.2f });

            bulletObjs_[i]->Update();
            bulletObjs_[i]->Draw();
        }
    }

    // --- ミサイルの描画 ---
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive && missileObjs_[i]) {
            missileObjs_[i]->SetPosition(missiles_[i].position);

            Vector3 mRot = { 0.0f, std::atan2(missiles_[i].velocity.x, missiles_[i].velocity.z), 0.0f };
            float xzLen = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x + missiles_[i].velocity.z * missiles_[i].velocity.z);
            mRot.x = std::atan2(-missiles_[i].velocity.y, xzLen);
            missileObjs_[i]->SetRotate(mRot);

            Vector3 missileScale = { scale_.x * 0.4f, scale_.y * 0.4f, scale_.z * 0.4f };
            missileObjs_[i]->SetScale(missileScale);

            missileObjs_[i]->Update();
            missileObjs_[i]->Draw();
        }
    }

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
    col.center = translate_;
    col.center.y += 1.0f;
    col.radius = kColliderRadius;
    return col;
}

void Player::ApplyDamage(int damage) {
    if (isDead_ || invincibleTimer_ > 0) {
        return;
    }

    hp_ -= damage;
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
        OutputDebugStringA("Player Dead!\n");
    } else {
        invincibleTimer_ = 60;
        OutputDebugStringA("Player Damaged!\n");
    }
}
// ---------------------------------------------------------


void Player::HandleMovement() {
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    if (input_->IsKeyDown('W')) move.z += 1.0f;
    if (input_->IsKeyDown('S')) move.z -= 1.0f;
    if (input_->IsKeyDown('A')) move.x -= 1.0f;
    if (input_->IsKeyDown('D')) move.x += 1.0f;

    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);

        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        float moveX = move.x * cosY + move.z * sinY;
        float moveZ = -move.x * sinY + move.z * cosY;

        translate_.x += moveX * kMoveSpeed;
        translate_.z += moveZ * kMoveSpeed;
    }

    if (isGrounded_) {
        if (input_->IsKeyPressed(VK_SPACE)) {
            velocity_.y = kJumpForce;
            isGrounded_ = false;
        }
    } else {
        velocity_.y -= kGravity;
        translate_.y += velocity_.y;

        if (translate_.y <= 0.0f) {
            translate_.y = 0.0f;
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    }
}

void Player::HandleAttack() {
    if (input_->IsKeyPressed('P') && !attackCollision_.isActive) {
        attackCollision_.isActive = true;
        attackActiveTimer_ = 10;
        OutputDebugStringA("Player Attack Start!\n");
    }

    if (attackCollision_.isActive) {
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        attackCollision_.center.x = translate_.x + sinY * 1.5f;
        attackCollision_.center.y = translate_.y + 1.0f;
        attackCollision_.center.z = translate_.z + cosY * 1.5f;

        if (attackObj_) {
            attackObj_->SetPosition(attackCollision_.center);
            attackObj_->SetRotate(rotate_);
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
    if (missileCooldown_ > 0) {
        missileCooldown_--;
    }

    // Mキーで発射
    if (input_->IsKeyPressed('M') && missileCooldown_ <= 0) {
        missileCooldown_ = 60;

        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);

        for (int i = 0; i < kMaxMissiles; ++i) {
            missiles_[i].isActive = true;
            missiles_[i].timer = 120;

            missiles_[i].target = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };

            missiles_[i].position = {
                translate_.x + sinY * 1.0f,
                translate_.y + 1.0f,
                translate_.z + cosY * 1.0f
            };

            float spreadX = ((std::rand() % 100) / 25.0f) - 2.0f;
            float spreadY = ((std::rand() % 100) / 25.0f) - 0.5f;
            float spreadZ = ((std::rand() % 100) / 25.0f) - 2.0f;

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

            Vector3 toTarget = {
                missiles_[i].target.x - missiles_[i].position.x,
                missiles_[i].target.y - missiles_[i].position.y,
                missiles_[i].target.z - missiles_[i].position.z
            };

            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
            if (dist > 0.001f) {
                toTarget.x /= dist;
                toTarget.y /= dist;
                toTarget.z /= dist;
            }

            // 誘導の強さ
            float turnSpeed = 0.08f;
            missiles_[i].velocity.x += toTarget.x * turnSpeed;
            missiles_[i].velocity.y += toTarget.y * turnSpeed;
            missiles_[i].velocity.z += toTarget.z * turnSpeed;

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

            missiles_[i].position.x += missiles_[i].velocity.x;
            missiles_[i].position.y += missiles_[i].velocity.y;
            missiles_[i].position.z += missiles_[i].velocity.z;

            missiles_[i].timer--;
            if (missiles_[i].timer <= 0) {
                missiles_[i].isActive = false;
            }
        }
    }
}

void Player::HandleMachineGun() {
    if (input_->IsKeyPressed('F') && machineGunActiveTimer_ <= 0) {
        machineGunActiveTimer_ = 180;
        machineGunFireTimer_ = 0;
        OutputDebugStringA("MachineGun Start!\n");
    }

    if (machineGunActiveTimer_ > 0) {
        machineGunActiveTimer_--;
        machineGunFireTimer_--;

        if (machineGunFireTimer_ <= 0) {
            machineGunFireTimer_ = 6;

            float sinY = std::sin(rotate_.y);
            float cosY = std::cos(rotate_.y);
            float rightX = cosY;
            float rightZ = -sinY;

            Vector3 leftShoulder = { translate_.x - rightX * 0.7f, translate_.y + 1.0f, translate_.z - rightZ * 0.7f };
            Vector3 rightShoulder = { translate_.x + rightX * 0.7f, translate_.y + 1.0f, translate_.z + rightZ * 0.7f };

            FireMachineGunBullet(leftShoulder);
            FireMachineGunBullet(rightShoulder);
        }
    }

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
            bullets_[i].timer = 60;

            Vector3 playerCenter = { translate_.x, translate_.y + 1.0f, translate_.z };
            Vector3 aimPos = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };
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
                float cosPitch = std::cos(cameraPitch_);
                float sinPitch = std::sin(cameraPitch_);
                toTarget = { std::sin(rotate_.y) * cosPitch, -sinPitch, std::cos(rotate_.y) * cosPitch };
            }

            float bulletSpeed = 3.0f;
            bullets_[i].velocity = {
                toTarget.x * bulletSpeed,
                toTarget.y * bulletSpeed,
                toTarget.z * bulletSpeed
            };
            break;
        }
    }
}

void Player::UpdateCamera() {
    if (!camera_) return;

    Vector3 cameraPos;
    const float kCameraJumpFollowRatio = 0.8f;

    if (viewMode_ == ViewMode::kThirdPerson) {
        float distance = 5.0f;
        float heightOffset = 1.5f + (translate_.y * kCameraJumpFollowRatio);

        float cosPitch = std::cos(cameraPitch_);
        float sinPitch = std::sin(cameraPitch_);
        float cosYaw = std::cos(rotate_.y);
        float sinYaw = std::sin(rotate_.y);

        cameraPos.x = translate_.x - (sinYaw * cosPitch * distance);
        cameraPos.y = heightOffset - (sinPitch * distance);
        cameraPos.z = translate_.z - (cosYaw * cosPitch * distance);

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    } else {
        cameraPos.x = translate_.x;
        cameraPos.y = 1.0f + (translate_.y * kCameraJumpFollowRatio);
        cameraPos.z = translate_.z;

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    }
}