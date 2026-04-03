#include "Player.h"
#include "camera/Camera.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include <cstdio> 
#include "Engine/Core/Math/Geometry/Math.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "../enemy/Enemy.h" 

#ifdef USE_IMGUI
#include <imgui.h> 
#endif

Player::~Player() {
}

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine) {
    input_ = input;
    engine_ = engine;

    movement_.Initialize();
    weapon_.Initialize(camera);
    cameraController_.Initialize(camera);
    status_.Initialize();

    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(camera, "enemy/body.obj");
    obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    attackObj_ = std::make_unique<ObjClass>();
    attackObj_->Initialize(camera, "enemy/body.obj");
    attackObj_->SetPosition(translate_);
    attackObj_->Update();

    targetMarkerObj_ = std::make_unique<ObjClass>();
    targetMarkerObj_->Initialize(camera, "enemy/body.obj");
    targetMarkerObj_->SetColor({ 0.0f, 1.0f, 0.0f, 0.5f });
    targetMarkerObj_->SetScale({ 0.5f, 0.5f, 0.5f });

    maskSprite_ = std::make_unique<Sprite>();
    maskSprite_->Initialize(camera, "resources/texture/player/mask.png");

    skillDurationTimer_ = 0;
    skillCooldownTimer_ = 0;
    karakuriChargeTimer_ = 0;
    karakuriActiveTimer_ = 0;
    isKarakuriCharged_ = false;

    attackState_ = AttackState::kNone;
    chargeTimer_ = 0;
    currentChargeRate_ = 0.0f;

    attackCollision_.center = translate_;
    attackCollision_.isActive = false;
    attackCollision_.radius = 0.0f;

    // 死亡演出用変数の初期化
    deathTimer_ = 0;
    deathVelocity_ = { 0.0f, 0.0f, 0.0f };
    deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
    deathYaw_ = 0.0f;
    isDeathAnimationFinished_ = false;

#ifdef USE_IMGUI
    lineOBB_ = std::make_unique<Line3DRegion>();
    lineOBB_->Initialize(camera);
#endif
}

void Player::Update() {
    // ====== 死亡時のド派手な吹き飛び演出 ======
    if (status_.IsDead()) {
        if (deathTimer_ == 0) {
            // 死亡した瞬間の向きを保存（カメラ用）
            deathYaw_ = rotate_.y;

            // プレイヤーの背面方向かつ上方向に吹き飛ばす
            float backwardSpeed = 1.5f + (std::rand() % 100) / 100.0f; // 1.5 ~ 2.5
            float upwardSpeed = 2.0f + (std::rand() % 100) / 100.0f;   // 2.0 ~ 3.0

            float sinY = std::sin(deathYaw_);
            float cosY = std::cos(deathYaw_);

            deathVelocity_.x = -sinY * backwardSpeed;
            deathVelocity_.y = upwardSpeed;
            deathVelocity_.z = -cosY * backwardSpeed;

            // ランダムに少し横ブレさせる
            deathVelocity_.x += ((std::rand() % 100) / 100.0f - 0.5f);
            deathVelocity_.z += ((std::rand() % 100) / 100.0f - 0.5f);

            // 激しいきりもみ回転
            deathAngularVelocity_.x = 0.2f + ((std::rand() % 100) / 500.0f);
            deathAngularVelocity_.y = 0.4f + ((std::rand() % 100) / 500.0f);
            deathAngularVelocity_.z = 0.2f + ((std::rand() % 100) / 500.0f);
        }

        deathTimer_++;

        // 指定したフレーム数が経過したら演出終了フラグを立てる
        if (deathTimer_ >= kDeathAnimationDuration) {
            isDeathAnimationFinished_ = true;
        }

        // 重力を適用して放物線を描かせる
        deathVelocity_.y -= 0.1f;

        // 速度を適用
        translate_.x += deathVelocity_.x;
        translate_.y += deathVelocity_.y;
        translate_.z += deathVelocity_.z;

        // 回転を適用
        rotate_.x += deathAngularVelocity_.x;
        rotate_.y += deathAngularVelocity_.y;
        rotate_.z += deathAngularVelocity_.z;

        // 死亡時専用のカメラワークを呼ぶ
        cameraController_.UpdateDeathCamera(translate_, deathYaw_, deathTimer_);

        // 死亡時はここで処理を終え、通常の移動や攻撃はスキップする
        return;
    }
    // ==========================================

    status_.Update();
    movement_.UpdateTimers();

#ifdef USE_IMGUI
    ImGui::Begin("Player");

    ImGui::Text("Player Status");
    float hpFraction = static_cast<float>(status_.GetHp()) / static_cast<float>(status_.GetMaxHp());
    if (hpFraction < 0.0f) hpFraction = 0.0f;

    char hpText[32];
    snprintf(hpText, sizeof(hpText), "HP: %d / %d", status_.GetHp(), status_.GetMaxHp());

    ImVec4 hpColor;
    if (hpFraction > 0.5f) hpColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
    else if (hpFraction > 0.2f) hpColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
    else hpColor = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
    ImGui::ProgressBar(hpFraction, ImVec2(-1.0f, 0.0f), hpText);
    ImGui::PopStyleColor();

    ImGui::Separator();

    if (ImGui::BeginTabBar("PlayerTabs")) {
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::SliderFloat("Mouse Sensitivity", cameraController_.GetMouseSensitivityPtr(), 0.0f, 100.0f);
            ImGui::DragFloat("Sensitivity Multiplier", cameraController_.GetMouseSensitivityMultiplierPtr(), 0.01f, 0.0f, 1.0f, "%.4f");
            ImGui::Checkbox("Camera Control Enabled", cameraController_.GetCameraControlEnabledPtr());

            if (skillDurationTimer_ > 0) {
                ImGui::Text("Skill ACTIVE (Firing): %d", skillDurationTimer_);
            } else {
                ImGui::Text("Skill Cooldown: %d / %d", skillCooldownTimer_, kSkillCooldownTime);
            }

            if (isKarakuriCharged_) {
                ImGui::Text("Karakuri State: MAX (Kaioken) - Time Left: %d", karakuriActiveTimer_);
                ImGui::Text("Dodge Cooldown: %d / %d", movement_.GetDodgeCooldownTimer(), movement_.GetMaxDodgeCooldownTime());
            } else {
                ImGui::Text("Karakuri Charge: %d / %d", karakuriChargeTimer_, kKarakuriChargeTime);
                ImGui::Text("Karakuri State: Normal");
            }

            ImGui::DragFloat("MachineGun Vibe Scale", weapon_.GetMachineGunVibrationScalePtr(), 0.001f, 0.0f, 0.5f);
            ImGui::DragFloat("Missile Vibe Scale", weapon_.GetMissileVibrationScalePtr(), 0.001f, 0.0f, 1.0f);

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Model")) {
            if (obj_) obj_->DebugTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
#endif

    cameraController_.UpdateInput(input_, rotate_);

    if (!isTargetingEnemy_) {
        float sinY = std::sin(rotate_.y);
        float cosY = std::cos(rotate_.y);
        aimPos_ = { translate_.x + sinY * kAimDistance, translate_.y, translate_.z + cosY * kAimDistance };
    } else {
        aimPos_ = targetPos_;
    }

    if (targetMarkerObj_) {
        targetMarkerObj_->SetPosition(aimPos_);
        targetMarkerObj_->Update();
    }

    HandleMovement();
    HandleAttack();
    HandleSkill();

    weapon_.Update(translate_, rotate_, cameraController_.GetCameraPitch(), aimPos_, scale_);
    cameraController_.Update(translate_, rotate_, weapon_.GetMissileVibration());
    status_.UpdateKnockback();

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

            PlayerCollider col = status_.GetCollider(translate_, rotate_, weapon_.GetMissileVibration());
            addSphereLines(col.center, col.radius, greenColor);

            if (attackCollision_.isActive && cameraController_.IsCameraControlEnabled()) {
                addSphereLines(attackCollision_.center, attackCollision_.radius, greenColor);
            }

            MissileData* ms = weapon_.GetMissiles();
            for (int i = 0; i < PlayerWeapon::GetMaxMissiles(); ++i) {
                if (ms[i].isActive) addSphereLines(ms[i].position, 2.0f, greenColor);
            }
            MachineGunBullet* mbs = weapon_.GetMachineGunBullets();
            for (int i = 0; i < PlayerWeapon::GetMaxMachineGunBullets(); ++i) {
                if (mbs[i].isActive) addSphereLines(mbs[i].position, 1.0f, greenColor);
            }
        }
        lineOBB_->Update();
    }
#endif
}

void Player::Draw() {
    bool isBlinking = (status_.GetInvincibleTimer() > 0 && (status_.GetInvincibleTimer() % 4) < 2);

    if (obj_) {
        if (isKarakuriCharged_) {
            obj_->SetColor({ 1.0f, 0.8f, 0.0f, 1.0f });
        } else if (status_.IsDead()) {
            obj_->SetColor({ 0.3f, 0.3f, 0.3f, 1.0f }); // 死亡時は少し暗くする
        } else {
            obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        }

        Vector3 drawPos = translate_;
        // ★死亡時にはミサイルの振動を反映させない（きりもみ回転が見づらくなるため）
        if (!status_.IsDead()) {
            drawPos += weapon_.GetMissileVibration();
        }
        drawPos.y += kModelOffsetY;

        obj_->SetPosition(drawPos);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        // ★修正：死亡時は一人称視点モードでも、点滅中（無敵時間）でも強制的にモデルを描画する
        if (status_.IsDead()) {
            obj_->Draw();
        } else if (!cameraController_.IsFirstPerson() && !isBlinking) {
            obj_->Draw();
        }
    }

    if (attackObj_ && attackState_ != AttackState::kNone && !status_.IsDead() && cameraController_.IsCameraControlEnabled()) {
        attackObj_->Draw();
    }

    if (isTargetingEnemy_ && targetMarkerObj_ && !status_.IsDead()) {
        targetMarkerObj_->Draw();
    }

    weapon_.Draw(translate_, rotate_, cameraController_.GetCameraPitch(), aimPos_, static_cast<int>(cameraController_.GetViewMode()), isBlinking, status_.IsDead());

    if (cameraController_.IsFirstPerson() && !status_.IsDead()) {
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

void Player::DrawParticles() {
    weapon_.DrawParticles(engine_);
}

bool Player::ApplyDamage(int damage) {
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;

    int finalDamage = damage;
    if (isCharging) {
        finalDamage *= 2;
    }

    return status_.ApplyDamage(finalDamage, false, engine_);
}

void Player::HandleMovement() {
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;

    int currentInvincible = status_.GetInvincibleTimer();
    movement_.Update(input_, isCharging, isKarakuriCharged_, translate_, rotate_, currentInvincible);

    if (currentInvincible > status_.GetInvincibleTimer()) {
        status_.SetInvincibleTimer(currentInvincible);
    }
}

void Player::HandleAttack() {
#ifdef USE_IMGUI
    if (!engine_->IsCursorLocked() && ImGui::GetIO().WantCaptureMouse) return;
#endif

    if (!cameraController_.IsCameraControlEnabled()) {
        attackState_ = AttackState::kNone;
        attackCollision_.isActive = false;
        attackActiveTimer_ = 0;
        return;
    }

    bool isLButtonDown = input_->IsMouseButtonDown(Mouse::Button::Left);

    switch (attackState_) {
    case AttackState::kNone:
        if (input_->IsMouseButtonPressed(Mouse::Button::Left)) {
            attackState_ = AttackState::kCharging;
            chargeTimer_ = 0;
        }
        break;

    case AttackState::kCharging:
        if (isLButtonDown) {
            chargeTimer_++;
            float chargeRate = static_cast<float>(chargeTimer_) / kMaxChargeTime;
            if (chargeRate > 1.0f) chargeRate = 1.0f;

            float currentAngle = rotate_.y + kHammerAngleOffset;
            float sinA = std::sin(currentAngle);
            float cosA = std::cos(currentAngle);
            float swingRadius = kSwingBaseRadius;
            float hammerHeight = kHammerBaseHeight + (std::sin(static_cast<float>(chargeTimer_) * kHammerSwaySpeed) * kHammerSwayAmplitude * chargeRate);

            Vector3 hammerPos;
            hammerPos.x = translate_.x + sinA * swingRadius;
            hammerPos.y = translate_.y + hammerHeight;
            hammerPos.z = translate_.z + cosA * swingRadius;

            if (attackObj_) {
                attackObj_->SetPosition(hammerPos + weapon_.GetMissileVibration());
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = kHammerRotX;
                attackObj_->SetRotate(swingRot);
                float hammerSize = kHammerBaseSize + (chargeRate * kHammerSizeChargeBonus);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * kHammerScaleYMultiplier * hammerSize, scale_.z * hammerSize };
                attackObj_->SetScale(hammerScale);
                attackObj_->Update();
            }
        } else {
            attackState_ = AttackState::kAttacking;
            attackActiveTimer_ = kAttackDuration;
            attackCollision_.isActive = true;
            currentChargeRate_ = static_cast<float>(chargeTimer_) / kMaxChargeTime;
            if (currentChargeRate_ > 1.0f) currentChargeRate_ = 1.0f;

            float hammerSize = kHammerBaseSize + (currentChargeRate_ * kHammerSizeChargeBonus);
            attackCollision_.radius = hammerSize;
        }
        break;

    case AttackState::kAttacking:
        if (attackActiveTimer_ > 0) {
            float t = 1.0f - (static_cast<float>(attackActiveTimer_) / kAttackDuration);
            float swingAngleOffset = kHammerAngleOffset - (kSwingTotalAngle * t);
            float currentAngle = rotate_.y + swingAngleOffset;
            float sinA = std::sin(currentAngle);
            float cosA = std::cos(currentAngle);

            float swingRadius = kSwingBaseRadius + (currentChargeRate_ * kSwingRadiusChargeBonus);
            float hammerHeight = kHammerBaseHeight;

            attackCollision_.center.x = translate_.x + sinA * swingRadius;
            attackCollision_.center.y = translate_.y + hammerHeight;
            attackCollision_.center.z = translate_.z + cosA * swingRadius;

            if (attackObj_) {
                attackObj_->SetPosition(attackCollision_.center + weapon_.GetMissileVibration());
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = kHammerRotX;
                attackObj_->SetRotate(swingRot);
                float hammerSize = kHammerBaseSize + (currentChargeRate_ * kHammerSizeChargeBonus);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * kHammerScaleYMultiplier * hammerSize, scale_.z * hammerSize };
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
        if (skillDurationTimer_ <= 0) skillCooldownTimer_ = kSkillCooldownTime;
    } else if (skillCooldownTimer_ > 0) {
        skillCooldownTimer_--;
    }

    if (isKarakuriCharged_) {
        karakuriActiveTimer_--;
        if (karakuriActiveTimer_ <= 0) {
            isKarakuriCharged_ = false;
            OutputDebugStringA("Karakuri Charge Ended.\n");
        }
    }

    if (input_->IsKeyDown('E')) {
        if (!isKarakuriCharged_) {
            karakuriChargeTimer_++;
            if (karakuriChargeTimer_ >= kKarakuriChargeTime) {
                isKarakuriCharged_ = true;
                karakuriChargeTimer_ = 0;
                karakuriActiveTimer_ = kKarakuriActiveTime;
            }
        }
    } else {
        if (!isKarakuriCharged_) karakuriChargeTimer_ = 0;
    }

#ifdef USE_IMGUI
    if (!engine_->IsCursorLocked() && ImGui::GetIO().WantCaptureMouse) return;
#endif

    if (!cameraController_.IsCameraControlEnabled()) return;

    if (input_->IsMouseButtonPressed(Mouse::Button::Right)) {
        if (skillDurationTimer_ <= 0 && skillCooldownTimer_ <= 0) {
            if (isKarakuriCharged_) {
                int fireCount = isTargetingEnemy_ ? 2 : 1;
                for (int i = 0; i < fireCount; ++i) {
                    weapon_.FireMissileSkill(translate_, rotate_, targetPos_);
                }
                skillDurationTimer_ = kMissileSkillDuration;
            } else {
                weapon_.StartMachineGunSkill();
                skillDurationTimer_ = kMachineGunSkillDuration;
            }
        }
    }
}

void Player::HitAndKnockback(Enemy* enemy) {
    status_.HitAndKnockback(enemy, translate_);
}