#include "Player.h"
#include "camera/Camera.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include <cstdio> 
#include "Engine/Core/Math/Math.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "../enemy/Enemy.h" 
#include "contents/ui/PlayerHPBar.h"

#ifdef USE_IMGUI
#include <imgui.h> 
#endif

Player::~Player() {
}

void Player::Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine) {
    input_ = input;
    engine_ = engine;
    camera_ = camera;

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

    // ★追加: キラン☆演出用 plane.obj の初期化
    starObj_ = std::make_unique<ObjClass>();
    starObj_->Initialize(camera, "plane.obj"); // ユーザー指定の plane.obj
    starObj_->SetColor({ 5.0f, 5.0f, 1.0f, 1.0f }); // 光る黄色に設定
    starScale_ = { 0.0f, 0.0f, 0.0f };
    starRotationZ_ = 0.0f;

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

    scale_ = { 0.3f, 0.5f, 0.3f };

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

    hpBar_ = std::make_unique<PlayerHPBar>();
    hpBar_->Initialize(camera_);
}

void Player::Update() {
    // ====== 死亡時の敵目線＆彼方へ消え去る演出 ======
    if (status_.IsDead()) {
        if (deathTimer_ == 0) {
            deathYaw_ = rotate_.y;

            // 敵と密着していてもめり込まないように、
            // 「死亡した瞬間のプレイヤーから見て前方40、高さ20」にカメラを固定
            float sY = std::sin(deathYaw_);
            float cY = std::cos(deathYaw_);
            deathCameraPos_ = {
                translate_.x + sY * 40.0f,
                translate_.y + 20.0f,
                translate_.z + cY * 40.0f
            };

            // 敵から見て奥（プレイヤーの背面斜め上）へ吹っ飛ぶ
            float backwardSpeed = 0.8f + (std::rand() % 100) / 100.0f;
            float upwardSpeed = 1.5f + (std::rand() % 100) / 100.0f;

            deathVelocity_.x = -sY * backwardSpeed;
            deathVelocity_.y = upwardSpeed;
            deathVelocity_.z = -cY * backwardSpeed;

            deathAngularVelocity_.x = 0.8f;
            deathAngularVelocity_.y = 1.2f;
            deathAngularVelocity_.z = 0.5f;
        }

        deathTimer_++;

        int flashTime = kDeathAnimationDuration - 40; // 終了の40フレーム前に光らせる

        if (deathTimer_ < flashTime) {
            deathVelocity_.y += 0.02f; // 上へ加速

            // 遠近感を強調するため、少し経ってから徐々にモデルのスケールを小さくしていく
            if (deathTimer_ > 30) {
                scale_.x *= 0.96f;
                scale_.y *= 0.96f;
                scale_.z *= 0.96f;
            }
        } else if (deathTimer_ == flashTime) {
            // 星になる瞬間！ピタッと止まる
            deathVelocity_ = { 0.0f, 0.0f, 0.0f };
            deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
            scale_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー本体は消す

            // ★plane.objを使って星の演出を開始！
            starScale_ = { 6.0f, 6.0f, 6.0f }; // 最初は大きく表示
            starRotationZ_ = 0.0f;
        } else if (deathTimer_ > flashTime) {
            // ★plane.objを回転させながら徐々に小さくする
            starRotationZ_ += 0.5f; // くるくる回す速度
            starScale_.x *= 0.88f;  // シュッと小さくしていく
            starScale_.y *= 0.88f;
            starScale_.z *= 0.88f;
        }

        translate_.x += deathVelocity_.x;
        translate_.y += deathVelocity_.y;
        translate_.z += deathVelocity_.z;

        // 天球を超えないように制限
        if (translate_.y > 80.0f) translate_.y = 80.0f;
        float limitXZ = 95.0f;
        if (translate_.x > limitXZ) translate_.x = limitXZ;
        if (translate_.x < -limitXZ) translate_.x = -limitXZ;
        if (translate_.z > limitXZ) translate_.z = limitXZ;
        if (translate_.z < -limitXZ) translate_.z = -limitXZ;

        rotate_.x += deathAngularVelocity_.x;
        rotate_.y += deathAngularVelocity_.y;
        rotate_.z += deathAngularVelocity_.z;

        if (deathTimer_ >= kDeathAnimationDuration) {
            isDeathAnimationFinished_ = true;
        }

        // 敵の目線から、プレイヤーの座標を見つめ続ける
        cameraController_.UpdateDeathCamera(deathCameraPos_, translate_);

        // カメラ更新後にパーティクルのみ更新し、WVP行列を最新化する
        weapon_.UpdateParticlesOnly();

        // ★星モデルの座標と回転（ビルボード）を更新
        if (starObj_ && deathTimer_ >= flashTime) {
            // カメラから星へのベクトルを計算して、カメラの方を向かせる（LookAt）
            Vector3 toCamera = {
                deathCameraPos_.x - translate_.x,
                deathCameraPos_.y - translate_.y,
                deathCameraPos_.z - translate_.z
            };
            float lookYaw = std::atan2(toCamera.x, toCamera.z);
            float horizontalDist = std::sqrt(toCamera.x * toCamera.x + toCamera.z * toCamera.z);
            float lookPitch = -std::atan2(toCamera.y, horizontalDist);

            starObj_->SetPosition(translate_);
            // XとYの回転でカメラの方向を向きつつ、Zの回転でくるくる回す！
            // ※もしplane.objの裏面が描画されず見えない場合は、lookYaw に 3.14159f を足してください
            starObj_->SetRotate({ lookPitch, lookYaw, starRotationZ_ });
            starObj_->SetScale(starScale_);
            starObj_->Update();
        }

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

    // プレイヤーとカメラの更新が全て終わった「最新の座標」でUIを更新し、ガタつきを防ぐ
    if (hpBar_) {
        hpBar_->Update(this, camera_);
    }

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

void Player::Draw3DUI() {
    if (hpBar_ && !status_.IsDead()) {
        hpBar_->Draw();
    }
}

void Player::Draw() {
    if (engine_) {
        engine_->ApplyPSO();
    }
    bool isBlinking = (status_.GetInvincibleTimer() > 0 && (status_.GetInvincibleTimer() % 4) < 2);

    if (obj_) {
        if (isKarakuriCharged_) {
            obj_->SetColor({ 1.0f, 0.8f, 0.0f, 1.0f });
        } else if (status_.IsDead()) {
            obj_->SetColor({ 0.15f, 0.15f, 0.15f, 1.0f }); // 飛んでいる間はシルエット
        } else {
            obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });
        }

        Vector3 drawPos = translate_;
        if (!status_.IsDead()) {
            drawPos += weapon_.GetMissileVibration();
        }
        drawPos.y += kModelOffsetY;

        obj_->SetPosition(drawPos);
        obj_->SetRotate(rotate_);
        obj_->SetScale(scale_);
        obj_->Update();

        if (status_.IsDead()) {
            // 星になる前まではプレイヤー本体を描画する
            int flashTime = kDeathAnimationDuration - 40;
            if (deathTimer_ < flashTime) {
                obj_->Draw();
            }
        } else if (!cameraController_.IsFirstPerson() && !isBlinking) {
            obj_->Draw();
        }
    }

    // ★追加: 星（plane.obj）の描画
    if (status_.IsDead() && starObj_ && deathTimer_ >= kDeathAnimationDuration - 40 && starScale_.x > 0.01f) {
        starObj_->Draw();
    }

    if (attackObj_ && attackState_ != AttackState::kNone && !status_.IsDead() && cameraController_.IsCameraControlEnabled()) {
        attackObj_->Draw();
    }

    if (isTargetingEnemy_ && targetMarkerObj_ && !status_.IsDead()) {
        // targetMarkerObj_->Draw();
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