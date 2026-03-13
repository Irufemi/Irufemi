#include "Player.h"

#include "camera/Camera.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include <cstdio> 
#include "Math.h"
#include "Engine/Platform/Input/Mouse.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "actors/enemy/Enemy.h" 

#ifdef USE_IMGUI
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

    // --- 機関銃の弾モデルの初期化 ---
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

    // スキル用変数の初期化
    skillDurationTimer_ = 0;
    skillCooldownTimer_ = 0;
    karakuriChargeTimer_ = 0;
    karakuriActiveTimer_ = 0; // ★追加
    isKarakuriCharged_ = false;

    // 近接攻撃判定の初期化
    attackState_ = AttackState::kNone;
    chargeTimer_ = 0;
    currentChargeRate_ = 0.0f;
    attackCollision_.isActive = false;
    attackCollision_.radius = 2.0f;

    // --- プレイヤーステータスの初期化 ---
    hp_ = kMaxHp;
    isDead_ = false;
    invincibleTimer_ = 0;

    // --- ノックバック変数の初期化 ---
    knockbackTarget_ = nullptr;
    knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
    knockbackTimer_ = 0;

#ifdef USE_IMGUI
    lineOBB_ = std::make_unique<Line3DRegion>();
    lineOBB_->Initialize(camera_);
#endif
    isCameraControlEnabled_ = true;
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

    // F2キーでカメラ操作の有効/無効を切り替え
    if (input_->IsKeyPressed(VK_F2)) {
        isCameraControlEnabled_ = !isCameraControlEnabled_;
    }

#ifdef USE_IMGUI
    ImGui::Begin("Player");

    // ==========================================
    // ImGuiでのHPバー描画
    // ==========================================
    ImGui::Text("Player Status");

    // HPの割合を計算 (0.0f ～ 1.0f)
    float hpFraction = static_cast<float>(hp_) / static_cast<float>(kMaxHp);
    if (hpFraction < 0.0f) hpFraction = 0.0f;

    // バーの上に表示するテキストを作成
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "HP: %d / %d", hp_, kMaxHp);

    // HPの残量によってバーの色を変える処理
    ImVec4 hpColor;
    if (hpFraction > 0.5f) {
        hpColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);      // 50%以上：緑色
    } else if (hpFraction > 0.2f) {
        hpColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);      // 20%以上：黄色（ピンチ）
    } else {
        hpColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);      // それ以下：赤色（大ピンチ）
    }

    // バーの色を適用してプログレスバーを描画
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
    ImGui::ProgressBar(hpFraction, ImVec2(-1.0f, 0.0f), hpText);
    ImGui::PopStyleColor();

    ImGui::Separator();
    // ==========================================

    if (ImGui::BeginTabBar("PlayerTabs")) {

        if (ImGui::BeginTabItem("Settings")) {
            ImGui::SliderFloat("Mouse Sensitivity", &mouseSensitivity_, 0.0f, 100.0f);
            ImGui::DragFloat("Sensitivity Multiplier", &mouseSensitivityMultiplier_, 0.01f, 0.0f, 1.0f, "%.4f");
            ImGui::Checkbox("Camera Control Enabled", &isCameraControlEnabled_);

            if (skillDurationTimer_ > 0) {
                ImGui::Text("Skill ACTIVE (Firing): %d", skillDurationTimer_);
            } else {
                ImGui::Text("Skill Cooldown: %d / %d", skillCooldownTimer_, kSkillCooldownTime);
            }

            // ★修正箇所：界王拳発動中かどうかで表示を変える
            if (isKarakuriCharged_) {
                ImGui::Text("Karakuri State: MAX (Kaioken) - Time Left: %d", karakuriActiveTimer_);
            } else {
                ImGui::Text("Karakuri Charge: %d / %d", karakuriChargeTimer_, kKarakuriChargeTime);
                ImGui::Text("Karakuri State: Normal");
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Model")) {
            if (obj_) {
                obj_->DebugTab();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

#endif

    // --- マウスによる視点操作 ---
    if (isCameraControlEnabled_ && mouse_) {
        Vector2 delta = mouse_->GetDelta();

        float sensitivityMult = mouseSensitivity_ * mouseSensitivityMultiplier_ * 0.001f;

        rotate_.y += delta.x * sensitivityMult;
        cameraPitch_ += delta.y * sensitivityMult;

        if (viewMode_ == ViewMode::kThirdPerson) {
            // 正の値が見下ろし、負の値が見上げです。
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -0.3f) cameraPitch_ = -0.3f;
        } else {
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -1.3f) cameraPitch_ = -1.3f;
        }
    }

    // 1. 移動処理
    HandleMovement();

    // 2. 近接攻撃処理（チャージ対応）
    HandleAttack();

    // 3. スキル・からくりチャージの入力管理（右クリック・Eキー）
    HandleSkill();

    // 4. ミサイルや機関銃の発射後の更新（移動処理など）
    UpdateMissile();
    UpdateMachineGun();

    // 5. 視点切り替え(Vキー)
    if (input_->IsKeyPressed('V')) {
        viewMode_ = (viewMode_ == ViewMode::kThirdPerson) ? ViewMode::kFirstPerson : ViewMode::kThirdPerson;

        // 視点を切り替えた直後の補正
        if (viewMode_ == ViewMode::kThirdPerson) {
            if (cameraPitch_ > 0.25f) cameraPitch_ = 0.25f;
            if (cameraPitch_ < -0.3f) cameraPitch_ = -0.3f;
        }
    }

    // 6. カメラをプレイヤーに追従させる
    UpdateCamera();

    // ==========================================
    // 敵を吹き飛ばす（ノックバック）処理
    // ==========================================
    if (knockbackTarget_ && knockbackTimer_ > 0) {
        Transform& enemyTransform = knockbackTarget_->GetGlobalTransform();

        enemyTransform.translate.x += knockbackVelocity_.x;
        enemyTransform.translate.y += knockbackVelocity_.y;
        enemyTransform.translate.z += knockbackVelocity_.z;

        // 摩擦
        knockbackVelocity_.x *= 0.85f;
        knockbackVelocity_.y *= 0.85f;
        knockbackVelocity_.z *= 0.85f;

        knockbackTimer_--;
        if (knockbackTimer_ <= 0) {
            knockbackTarget_ = nullptr;
        }
    }
    // ==========================================

#ifdef USE_IMGUI
    if (input_->IsKeyPressedDIK(0x3B /*DIK_F1*/)) {
        isDebugDrawOBB_ = !isDebugDrawOBB_;
    }

    if (lineOBB_) {
        lineOBB_->ClearInstances();
        if (isDebugDrawOBB_) {
            auto addSphereLines = [&](const Vector3& center, float radius, const Vector4& color) {
                const int segments = 16;
                const float step = (2.0f * 3.14159265f) / segments;

                for (int i = 0; i < segments; ++i) {
                    float theta1 = i * step;
                    float theta2 = (i + 1) * step;
                    Vector3 p1 = { center.x + radius * std::cos(theta1), center.y, center.z + radius * std::sin(theta1) };
                    Vector3 p2 = { center.x + radius * std::cos(theta2), center.y, center.z + radius * std::sin(theta2) };
                    lineOBB_->AddInstance(p1, p2, color);
                }
                for (int i = 0; i < segments; ++i) {
                    float theta1 = i * step;
                    float theta2 = (i + 1) * step;
                    Vector3 p1 = { center.x + radius * std::cos(theta1), center.y + radius * std::sin(theta1), center.z };
                    Vector3 p2 = { center.x + radius * std::cos(theta2), center.y + radius * std::sin(theta2), center.z };
                    lineOBB_->AddInstance(p1, p2, color);
                }
                for (int i = 0; i < segments; ++i) {
                    float theta1 = i * step;
                    float theta2 = (i + 1) * step;
                    Vector3 p1 = { center.x, center.y + radius * std::cos(theta1), center.z + radius * std::sin(theta1) };
                    Vector3 p2 = { center.x, center.y + radius * std::cos(theta2), center.z + radius * std::sin(theta2) };
                    lineOBB_->AddInstance(p1, p2, color);
                }
                };

            Vector4 greenColor = { 0.0f, 1.0f, 0.0f, 1.0f };

            PlayerCollider col = GetCollider();
            addSphereLines(col.center, col.radius, greenColor);

            if (attackCollision_.isActive && isCameraControlEnabled_) {
                addSphereLines(attackCollision_.center, attackCollision_.radius, greenColor);
            }
            for (int i = 0; i < kMaxMissiles; ++i) {
                if (missiles_[i].isActive) addSphereLines(missiles_[i].position, 2.0f, greenColor);
            }
            for (int i = 0; i < kMaxBullets; ++i) {
                if (bullets_[i].isActive) addSphereLines(bullets_[i].position, 1.0f, greenColor);
            }
        }
        lineOBB_->Update();
    }
#endif
}

void Player::Draw() {
    bool isBlinking = (invincibleTimer_ > 0 && (invincibleTimer_ % 4) < 2);

    if (obj_) {
        if (isKarakuriCharged_) {
            obj_->SetColor({ 1.0f, 0.8f, 0.0f, 1.0f });
        } else {
            obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        }

        obj_->SetPosition(translate_);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        if (viewMode_ != ViewMode::kFirstPerson && !isBlinking && !isDead_) {
            obj_->Draw();
        }
    }

    if (attackObj_ && attackState_ != AttackState::kNone && !isDead_ && isCameraControlEnabled_) {
        attackObj_->Draw();
    }

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

    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive && bulletObjs_[i] && !isDead_) {
            bulletObjs_[i]->SetPosition(bullets_[i].position);
            Vector3 bRot = { 0.0f, std::atan2(bullets_[i].velocity.x, bullets_[i].velocity.z), 0.0f };
            float bxzLen = std::sqrt(bullets_[i].velocity.x * bullets_[i].velocity.x + bullets_[i].velocity.z * bullets_[i].velocity.z);
            bRot.x = std::atan2(-bullets_[i].velocity.y, bxzLen);
            bulletObjs_[i]->SetRotate(bRot);
            bulletObjs_[i]->SetScale({ 0.05f, 0.05f, 0.2f });
            bulletObjs_[i]->Update();
            bulletObjs_[i]->Draw();
        }
    }

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
        if (maskSprite_) maskSprite_->Draw();
    }

#ifdef USE_IMGUI
    if (lineOBB_ && isDebugDrawOBB_ && engine_) {
        engine_->ApplyLineInstancedPSO();
        lineOBB_->Draw();
        engine_->ApplyPSO();
    }
#endif
}

PlayerCollider Player::GetCollider() const {
    PlayerCollider col;
    col.center = translate_;
    col.center.y += 0.2f;
    col.radius = kColliderRadius;
    col.obb.center = col.center;
    Matrix4x4 rotateMatrix = Math::MakeRotateMatrix(Math::MakeRotateAxisAngleQuaternion({ 0.0f, 1.0f, 0.0f }, rotate_.y));
    col.obb.orientations[0] = { rotateMatrix.m[0][0], rotateMatrix.m[0][1], rotateMatrix.m[0][2] };
    col.obb.orientations[1] = { rotateMatrix.m[1][0], rotateMatrix.m[1][1], rotateMatrix.m[1][2] };
    col.obb.orientations[2] = { rotateMatrix.m[2][0], rotateMatrix.m[2][1], rotateMatrix.m[2][2] };
    col.obb.size = { 0.3f, 0.3f, 0.3f };
    return col;
}

void Player::ApplyDamage(int damage) {
    if (isDead_ || invincibleTimer_ > 0) return;
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;
    int finalDamage = isCharging ? damage * 2 : damage;
    hp_ -= finalDamage;
    if (hp_ <= 0) {
        hp_ = 0;
        isDead_ = true;
    } else {
        invincibleTimer_ = 60;
    }
}

void Player::HandleMovement() {
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;
    Vector3 move = { 0.0f, 0.0f, 0.0f };

    if (!isCharging) {
        if (input_->IsKeyDown('W')) move.z += 1.0f;
        if (input_->IsKeyDown('S')) move.z -= 1.0f;
        if (input_->IsKeyDown('A')) move.x -= 1.0f;
        if (input_->IsKeyDown('D')) move.x += 1.0f;
    }

    if (move.x != 0.0f || move.z != 0.0f) {
        move = Math::Normalize(move);
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);
        float moveX = move.x * cosY + move.z * sinY;
        float moveZ = -move.x * sinY + move.z * cosY;

        translate_.x += moveX * kMoveSpeed;
        translate_.z += moveZ * kMoveSpeed;

        if (translate_.x > kFieldRangeX)  translate_.x = kFieldRangeX;
        if (translate_.x < -kFieldRangeX) translate_.x = -kFieldRangeX;
        if (translate_.z > kFieldRangeZ)  translate_.z = kFieldRangeZ;
        if (translate_.z < -kFieldRangeZ) translate_.z = -kFieldRangeZ;
    }

    if (isGrounded_) {
        if (!isCharging && input_->IsKeyPressed(VK_SPACE)) {
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
#ifdef USE_IMGUI
    if (ImGui::GetIO().WantCaptureMouse) return;
#endif

    if (!isCameraControlEnabled_) {
        attackState_ = AttackState::kNone;
        attackCollision_.isActive = false;
        attackActiveTimer_ = 0;
        return;
    }

    bool isLButtonDown = false;
    if (mouse_) isLButtonDown = mouse_->IsButtonDown(Mouse::Button::Left);

    switch (attackState_) {
    case AttackState::kNone:
        if (mouse_ && mouse_->IsButtonPressed(Mouse::Button::Left)) {
            attackState_ = AttackState::kCharging;
            chargeTimer_ = 0;
        }
        break;

    case AttackState::kCharging:
        if (isLButtonDown) {
            chargeTimer_++;
            float chargeRate = static_cast<float>(chargeTimer_) / 60.0f;
            if (chargeRate > 1.0f) chargeRate = 1.0f;

            float currentAngle = rotate_.y + 1.5f;
            float sinA = std::sin(currentAngle);
            float cosA = std::cos(currentAngle);
            float swingRadius = 2.5f;
            float hammerHeight = 1.0f + (std::sin(static_cast<float>(chargeTimer_) * 0.5f) * 0.1f * chargeRate);

            Vector3 hammerPos;
            hammerPos.x = translate_.x + sinA * swingRadius;
            hammerPos.y = translate_.y + hammerHeight;
            hammerPos.z = translate_.z + cosA * swingRadius;

            if (attackObj_) {
                attackObj_->SetPosition(hammerPos);
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = 1.57f;
                attackObj_->SetRotate(swingRot);
                float hammerSize = 0.8f + (chargeRate * 0.4f);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * 1.5f * hammerSize, scale_.z * hammerSize };
                attackObj_->SetScale(hammerScale);
                attackObj_->Update();
            }
        } else {
            attackState_ = AttackState::kAttacking;
            attackActiveTimer_ = kAttackDuration;
            attackCollision_.isActive = true;
            currentChargeRate_ = static_cast<float>(chargeTimer_) / 60.0f;
            if (currentChargeRate_ > 1.0f) currentChargeRate_ = 1.0f;
            attackCollision_.radius = 2.0f + (2.0f * currentChargeRate_);
        }
        break;

    case AttackState::kAttacking:
        if (attackActiveTimer_ > 0) {
            float t = 1.0f - (static_cast<float>(attackActiveTimer_) / kAttackDuration);
            float swingAngleOffset = 1.5f - (3.0f * t);
            float currentAngle = rotate_.y + swingAngleOffset;
            float sinA = std::sin(currentAngle);
            float cosA = std::cos(currentAngle);

            float swingRadius = 2.5f + (currentChargeRate_ * 0.5f);
            float hammerHeight = 1.0f;

            attackCollision_.center.x = translate_.x + sinA * swingRadius;
            attackCollision_.center.y = translate_.y + hammerHeight;
            attackCollision_.center.z = translate_.z + cosA * swingRadius;

            if (attackObj_) {
                attackObj_->SetPosition(attackCollision_.center);
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = 1.57f;
                attackObj_->SetRotate(swingRot);
                float hammerSize = 0.8f + (currentChargeRate_ * 0.4f);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * 1.5f * hammerSize, scale_.z * hammerSize };
                attackObj_->SetScale(hammerScale);
                attackObj_->Update();
            }

            attackActiveTimer_--;
            if (attackActiveTimer_ <= 0) {
                attackCollision_.isActive = false;
                attackState_ = AttackState::kNone;
            }
        }
        break;
    }
}

void Player::HandleSkill() {
    if (skillDurationTimer_ > 0) {
        skillDurationTimer_--;
        if (skillDurationTimer_ <= 0) {
            skillCooldownTimer_ = kSkillCooldownTime;
        }
    } else if (skillCooldownTimer_ > 0) {
        skillCooldownTimer_--;
    }

    // ★追加箇所：界王拳状態の持続時間（20秒）のカウントダウン
    if (isKarakuriCharged_) {
        karakuriActiveTimer_--;
        if (karakuriActiveTimer_ <= 0) {
            isKarakuriCharged_ = false; // 20秒経ったら通常状態に戻る
            OutputDebugStringA("Karakuri Charge Ended.\n");
        }
    }

    if (input_->IsKeyDown('E')) {
        if (!isKarakuriCharged_) {
            karakuriChargeTimer_++;
            if (karakuriChargeTimer_ >= kKarakuriChargeTime) {
                isKarakuriCharged_ = true;
                karakuriChargeTimer_ = 0;
                karakuriActiveTimer_ = kKarakuriActiveTime; // ★追加：持続時間をセット
            }
        }
    } else {
        if (!isKarakuriCharged_) karakuriChargeTimer_ = 0;
    }

#ifdef USE_IMGUI
    if (ImGui::GetIO().WantCaptureMouse) return;
#endif

    if (!isCameraControlEnabled_) return;

    if (mouse_ && mouse_->IsButtonPressed(Mouse::Button::Right)) {
        if (skillDurationTimer_ <= 0 && skillCooldownTimer_ <= 0) {
            if (isKarakuriCharged_) {
                FireMissileSkill();
                // ★削除箇所：ここで false にしていた処理を消し、タイマー終了まで持続するようにしました。
                // isKarakuriCharged_ = false; 
                skillDurationTimer_ = 120;
            } else {
                StartMachineGunSkill();
                skillDurationTimer_ = 180;
            }
        }
    }
}

void Player::FireMissileSkill() {
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
}

void Player::UpdateMissile() {
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
            if (missiles_[i].timer <= 0) missiles_[i].isActive = false;
        }
    }
}

void Player::StartMachineGunSkill() {
    machineGunActiveTimer_ = 180;
    machineGunFireTimer_ = 0;
}

void Player::UpdateMachineGun() {
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
            if (bullets_[i].timer <= 0) bullets_[i].isActive = false;
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

    Vector3 lookAtTarget = {
        translate_.x,
        translate_.y + 1.5f,
        translate_.z
    };

    if (viewMode_ == ViewMode::kThirdPerson) {

        float distance = 5.0f;

        float cosPitch = std::cos(cameraPitch_);
        float sinPitch = std::sin(cameraPitch_);
        float cosYaw = std::cos(rotate_.y);
        float sinYaw = std::sin(rotate_.y);

        cameraPos.x = lookAtTarget.x - (sinYaw * cosPitch * distance);
        cameraPos.y = lookAtTarget.y + (sinPitch * distance);
        cameraPos.z = lookAtTarget.z - (cosYaw * cosPitch * distance);

        if (cameraPos.y < 0.2f) {
            cameraPos.y = 0.2f;
        }

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    } else {
        cameraPos.x = translate_.x;
        cameraPos.y = 1.0f + (translate_.y * kCameraJumpFollowRatio);
        cameraPos.z = translate_.z;

        if (cameraPos.y < 0.2f) cameraPos.y = 0.2f;

        camera_->SetTranslate(cameraPos);
        camera_->SetRotate({ cameraPitch_, rotate_.y, 0.0f });
    }
}

void Player::HitAndKnockback(Enemy* enemy) {
    if (!enemy) return;

    knockbackTarget_ = enemy;
    knockbackTimer_ = 20;

    Vector3 pPos = translate_;
    Vector3 ePos = enemy->GetGlobalTransform().translate;
    Vector3 dir = { ePos.x - pPos.x, 0.0f, ePos.z - pPos.z };

    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.001f) {
        float power = 2.0f;
        knockbackVelocity_.x = (dir.x / len) * power;
        knockbackVelocity_.y = (dir.y / len) * power;
        knockbackVelocity_.z = (dir.z / len) * power;
    }
}