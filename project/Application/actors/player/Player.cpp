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

    // 各コンポーネントの初期化
    movement_.Initialize();
    weapon_.Initialize(camera);
    cameraController_.Initialize(camera);
    status_.Initialize();

    // --- モデルの生成と初期化 ---
    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(camera, "enemy/body.obj");
    obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    // --- 攻撃表示用モデルの生成と初期化 ---
    attackObj_ = std::make_unique<ObjClass>();
    attackObj_->Initialize(camera, "enemy/body.obj");

    // --- 一人称視点用マスク画像の生成と初期化 ---
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
    attackCollision_.center = {};
    attackCollision_.isActive = false;
    attackCollision_.radius = 1.0f;

#ifdef USE_IMGUI
    lineOBB_ = std::make_unique<Line3DRegion>();
    lineOBB_->Initialize(camera);
#endif
}

void Player::Update() {
    if (status_.IsDead()) return;

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

    HandleMovement();
    HandleAttack();
    HandleSkill();

    weapon_.Update(translate_, rotate_, cameraController_.GetCameraPitch(), targetPos_, scale_);
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
        } else {
            obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        }

        obj_->SetPosition(translate_ + weapon_.GetMissileVibration());
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);

        if (!cameraController_.IsFirstPerson() && !isBlinking && !status_.IsDead()) {
            obj_->Draw();
        }
    }

    if (attackObj_ && attackState_ != AttackState::kNone && !status_.IsDead() && cameraController_.IsCameraControlEnabled()) {
        attackObj_->Draw();
    }

    weapon_.Draw(translate_, rotate_, cameraController_.GetCameraPitch(), targetPos_, static_cast<int>(cameraController_.GetViewMode()), isBlinking, status_.IsDead());

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

void Player::ApplyDamage(int damage) {
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;
    status_.ApplyDamage(damage, isCharging, engine_);
}

void Player::HandleMovement() {
    bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;

    // ★修正箇所: 現在の無敵時間を取得して渡し、増えていたらセットする
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
                attackObj_->SetPosition(hammerPos + weapon_.GetMissileVibration());
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = 1.57f;
                attackObj_->SetRotate(swingRot);
                float hammerSize = 0.8f + (chargeRate * 0.4f);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * 1.5f * hammerSize, scale_.z * hammerSize };
                attackObj_->SetScale(hammerScale);
            }
        } else {
            attackState_ = AttackState::kAttacking;
            attackActiveTimer_ = kAttackDuration;
            attackCollision_.isActive = true;
            currentChargeRate_ = static_cast<float>(chargeTimer_) / 60.0f;
            if (currentChargeRate_ > 1.0f) currentChargeRate_ = 1.0f;

            float hammerSize = 0.8f + (currentChargeRate_ * 0.4f);
            attackCollision_.radius = hammerSize;
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
                attackObj_->SetPosition(attackCollision_.center + weapon_.GetMissileVibration());
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = 1.57f;
                attackObj_->SetRotate(swingRot);
                float hammerSize = 0.8f + (currentChargeRate_ * 0.4f);
                Vector3 hammerScale = { scale_.x * hammerSize, scale_.y * 1.5f * hammerSize, scale_.z * hammerSize };
                attackObj_->SetScale(hammerScale);
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
                weapon_.FireMissileSkill(translate_, rotate_, targetPos_);
                skillDurationTimer_ = 120;
            } else {
                weapon_.StartMachineGunSkill();
                skillDurationTimer_ = 180;
            }
        }
    }
}

void Player::HitAndKnockback(Enemy* enemy) {
    status_.HitAndKnockback(enemy, translate_);
}