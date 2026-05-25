#include "Player.h"
#include "Engine/Graphics/Camera/Camera.h"
#include <Windows.h>
#include <cmath>
#include <cstdlib>
#include <cstdio> 
#include "Engine/Core/Math/Math.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include "../enemy/Enemy.h" 
#include "contents/ui/PlayerHPBar.h"
#include "Renderer/Effect/WeaponTrail.h"
#include "Renderer/Effect/Effect.h"
#include "Renderer/ParticleGPU/GPUParticleSystem.h"
#include "actors/enemy/Beam/EnemyBeam.h"

#ifdef USE_IMGUI
#include <imgui.h> 
#endif

Player::Player() = default;

Player::~Player() {
}

void Player::Initialize(InputManager* input, IrufemiEngine* engine) {
    input_ = input;
    engine_ = engine;

    movement_.Initialize();
    weapon_.Initialize();
    cameraController_.Initialize();
    status_.Initialize();

    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize("enemy/body.obj");
    obj_->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f });

    attackObj_ = std::make_unique<ObjClass>();
    attackObj_->Initialize("player/playerMelee.obj");
    attackObj_->SetPosition(translate_);
    attackObj_->Update();

    targetMarkerObj_ = std::make_unique<ObjClass>();
    targetMarkerObj_->Initialize("enemy/body.obj");
    targetMarkerObj_->SetColor({ 0.0f, 1.0f, 0.0f, 0.5f });
    targetMarkerObj_->SetScale({ 0.5f, 0.5f, 0.5f });

    maskSprite_ = std::make_unique<Sprite>();
    maskSprite_->Initialize("resources/texture/player/mask.png");

    // ★追加: キラン☆演出用 plane.obj の初期化
    starObj_ = std::make_unique<ObjClass>();
    starObj_->Initialize("plane.obj"); // ユーザー指定の plane.obj
    starObj_->SetCustomPSO(engine->GetPSOManager()->GetPSO("Object3D", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Disable, PSOManager::CullMode::None));
    starObj_->SetColor({ 5.0f, 5.0f, 1.0f, 1.0f }); // 光る黄色に設定
    starScale_ = { 0.0f, 0.0f, 0.0f };
    starRotationZ_ = 0.0f;

    skillDurationTimer_ = 0;
    skillCooldownTimer_ = 0;
    karakuriChargeTimer_ = 0;
    karakuriActiveTimer_ = 0;
    isKarakuriCharged_ = false;
    cooldownWarningTimer_ = 0;

    attackState_ = AttackState::kNone;
    chargeTimer_ = 0;
    currentChargeRate_ = 0.0f;

    attackCollision_.center = translate_;
    attackCollision_.isActive = false;
    attackCollision_.radius = 0.0f;

    scale_ = { 0.3f, 0.5f, 0.3f };

    // 死亡演出用変数の初期化
    deathTimer_ = 0;
    deathWaitTimer_ = 0;
    deathVelocity_ = { 0.0f, 0.0f, 0.0f };
    deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
    deathYaw_ = 0.0f;
    isDeathAnimationFinished_ = false;

    aimingSprite_ = std::make_unique<Sprite>();
    aimingSprite_->Initialize("resources/texture/player/aiming.png");
    aimingSprite_->SetSize(96.0f, 96.0f);
    aimingSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 1.0f });
    aimingSprite_->SetPositionCenter(640.0f, 360.0f);

#ifdef USE_IMGUI
    lineOBB_ = std::make_unique<Line3DRegion>();
    lineOBB_->Initialize();
#endif

    hpBar_ = std::make_unique<PlayerHPBar>();
    hpBar_->Initialize(engine);

    weaponTrail_ = std::make_unique<WeaponTrail>();
    weaponTrail_->Initialize(engine, "resources/gradationLine.png", {1.0f, 0.6f, 0.1f, 1.0f});

    // ★追加: からくりチャージゲージの初期化
    karakuriGaugeBg_ = std::make_unique<Sprite>();
    karakuriGaugeBg_->Initialize("resources/whiteTexture.png");
    karakuriGaugeBg_->SetColor({ 0.2f, 0.2f, 0.0f, 0.5f }); // 暗い黄色
    karakuriGaugeBg_->SetSize(400.0f, 16.0f);
    // 中央下部（HPバーの少し上）に配置。画面幅1280, 高さ720想定
    karakuriGaugeBg_->SetPositionTopLeft(440.0f, 580.0f); 

    karakuriGaugeFill_ = std::make_unique<Sprite>();
    karakuriGaugeFill_->Initialize("resources/whiteTexture.png");
    karakuriGaugeFill_->SetColor({ 1.0f, 0.9f, 0.1f, 0.8f }); // 明るい黄色
    karakuriGaugeFill_->SetSize(0.0f, 16.0f); // 初期幅0
    karakuriGaugeFill_->SetPositionTopLeft(440.0f, 580.0f);

    // ★追加: 3D爆発エフェクトプールの事前生成
    explosionEffects_.clear();
    for (int i = 0; i < kMaxExplosionEffects; ++i) {
        auto effect = std::make_unique<Effect>();
        effect->Initialize(EffectType::kExplosion);
        explosionEffects_.push_back(std::move(effect));
    }


    // ★追加: チャージ中・完了時の上昇パーティクル
    karakuriChargeParticle_ = std::make_unique<GPUParticleSystem>();
    karakuriChargeParticle_->Initialize("resources/gradationLine.png");

    // ★追加: チャージしきったときの足元リングエフェクト
    karakuriRingParticle_ = std::make_unique<GPUParticleSystem>();
    karakuriRingParticle_->Initialize("resources/circle2.png");

    // ★追加: 死亡待機中の自爆前光線
    deathGlowParticle_ = std::make_unique<GPUParticleSystem>();
    deathGlowParticle_->Initialize("resources/gradationLine.png");

    // SEの初期化
    seHammer_ = std::make_unique<Se>();
    seHammer_->Initialize("resources/SE/player/hammer.mp3", "Player_Hammer", 0.2f);

    seHammerHit_ = std::make_unique<Se>();
    seHammerHit_->Initialize("resources/SE/player/hammer_hit.mp3", "Player_HammerHit", 0.2f);

    seMissileHit_ = std::make_unique<Se>();
    seMissileHit_->Initialize("resources/SE/player/missile.wav", "Player_MissileHit", 0.2f);

    seKarakuri_ = std::make_unique<Se>();
    seKarakuri_->Initialize("resources/SE/player/karakuri.wav", "Player_Karakuri", 0.2f, true);

    seCooldown_ = std::make_unique<Se>();
    seCooldown_->Initialize("resources/SE/player/cooldown.mp3", "Player_Cooldown", 0.2f);
}

void Player::Update() {
    // ====== 死亡時の待機 + 演出 ======
    if (status_.IsDead()) {
        // HPが0になってから3秒間（180フレーム）は演出を待機する
        if (deathWaitTimer_ < kDeathWaitTime) {
            deathWaitTimer_++;

            // === 死亡開始の最初のフレームでビームを生成 ===
            if (deathWaitTimer_ == 1) {
                deathBeams_.clear();
                deathBeamDirs_.clear();
                deathBeamDelays_.clear();
                deathBeamOffsets_.clear();
                
                // 15フレーム（0.25秒）刻みで順番に出現させる遅延テーブル
                deathBeamDelays_ = { 1, 16, 31, 46 };

                // ★実際の縮小モデルサイズ（高さ約1.0f, 幅約0.3f）に適合させたオフセット定義
                deathBeamOffsets_ = {
                    { 0.0f, 0.9f, 0.0f },     // ビーム0: 頭付近
                    { 0.12f, 0.65f, 0.03f },  // ビーム1: 右肩/右胸付近
                    { -0.12f, 0.4f, -0.03f }, // ビーム2: 左脇腹/左腰付近
                    { 0.08f, 0.15f, -0.02f }  // ビーム3: 右太もも/下半身付近
                };

                for (int i = 0; i < kDeathBeamCount; ++i) {
                    auto beam = std::make_unique<EnemyBeam>();
                    beam->Initialize(engine_);
                    beam->SetOriginOffset(0.0f);
                    
                    // 初期状態では非アクティブ。出現タイミングが来たらONにする
                    beam->SetAttackActive(false);
                    beam->SetChargeSphereActive(false); // コア球体は非表示
                    
                    // ★超発光HDRカラーに設定（1.0を超える強度で眩しく輝かせます）
                    beam->SetAttackColor({ 3.5f, 3.0f, 0.3f, 1.0f }); 
                    
                    // 3D球面上に均等・ランダムな方向を生成
                    float theta = (std::rand() % 1000) / 1000.0f * 3.14159f * 2.0f;
                    float phi = std::acos(2.0f * (std::rand() % 1000) / 1000.0f - 1.0f);
                    Vector3 dir = {
                        std::sin(phi) * std::cos(theta),
                        std::sin(phi) * std::sin(theta),
                        std::cos(phi)
                    };
                    
                    // 下半身から出るビームが地面に埋まりすぎないよう、Y方向を少し上向きに反転補正
                    if (deathBeamOffsets_[i].y < 0.5f && dir.y < 0.0f) {
                        dir.y = -dir.y * 0.5f; // 上向きにする
                        dir = Math::Normalize(dir);
                    }
                    
                    deathBeams_.push_back(std::move(beam));
                    deathBeamDirs_.push_back(dir);
                }
            }

            // === 毎フレームのビーム更新 ===
            if (!deathBeams_.empty()) {
                // ★モデル描画Yオフセット(kModelOffsetY = 0.4f)を足して、モデルと完全に同期させる！
                Vector3 basePos = translate_;
                basePos.y += kModelOffsetY;

                float sinY = std::sin(rotate_.y);
                float cosY = std::cos(rotate_.y);

                for (size_t i = 0; i < deathBeams_.size(); ++i) {
                    int delay = deathBeamDelays_[i];
                    if (deathWaitTimer_ >= delay) {
                        // 出現タイミングに達した場合のみ更新・表示
                        if (!deathBeams_[i]->IsAttackActive()) {
                            deathBeams_[i]->SetAttackActive(true);
                        }

                        // 出現してから終了までの固有の進行度を計算
                        int activeFrames = deathWaitTimer_ - delay;
                        int totalFrames = kDeathWaitTime - delay;
                        float beamRatio = static_cast<float>(activeFrames) / totalFrames;
                        if (beamRatio > 1.0f) beamRatio = 1.0f;

                        // 後半にかけて急激に太くする（出現時は 0.04f から最大 0.25f）
                        float curveRatio = std::pow(beamRatio, 2.0f);
                        float thickness = 0.04f + curveRatio * 0.21f;
                        deathBeams_[i]->SetAttackThickness(thickness);
                        
                        // ★プレイヤーの回転を考慮して、射出部位のオフセット座標を回転
                        Vector3 localOffset = deathBeamOffsets_[i];
                        Vector3 rotatedOffset = {
                            localOffset.x * cosY + localOffset.z * sinY,
                            localOffset.y,
                            -localOffset.x * sinY + localOffset.z * cosY
                        };
                        Vector3 startPos = basePos + rotatedOffset;
                        Vector3 endPos = startPos + deathBeamDirs_[i] * 300.0f;
                        
                        deathBeams_[i]->Update(startPos, endPos);
                    } else {
                        // まだ出現タイミングに達していないビームは更新しない
                        deathBeams_[i]->SetAttackActive(false);
                    }
                }
            }

            // ★追加: 死亡待機中（自爆前）の全身から激しく突き抜ける光線エフェクト
            if (deathGlowParticle_) {
                float deathRatio = static_cast<float>(deathWaitTimer_) / kDeathWaitTime;
                if (deathRatio > 1.0f) deathRatio = 1.0f;
                // 後半にかけて急激に激しくなるカーブ（2乗）
                float curveRatio = std::pow(deathRatio, 2.0f);

                Vector3 emitPos = translate_;
                emitPos.y += 1.0f; // プレイヤーの胸・胴体付近から放出

                // 球状エミッターから全方向に放出
                // 進行度に応じて、放出数と頻度を劇的に増やす
                uint32_t count = 3 + static_cast<uint32_t>(curveRatio * 80.0f);
                float freq = 0.05f - curveRatio * 0.046f;
                deathGlowParticle_->SetSphereEmitter(emitPos, 0.3f, count, freq);

                // 全方向に高速で突き抜ける光の筋
                deathGlowParticle_->SetDirection({ 0.0f, 0.0f, 0.0f });
                deathGlowParticle_->SetVelocity(12.0f + curveRatio * 28.0f);
                deathGlowParticle_->SetSpread(1.0f); // 放射速度をフルパワーに
                deathGlowParticle_->SetBillboardMode(2); // 進行方向ビルボード (Velocity Billboard) を適用！
                deathGlowParticle_->SetJitter(0.0f);
                deathGlowParticle_->SetGravity(0.0f);
                deathGlowParticle_->SetDamping(0.02f); // 滑らかに伸びる

                // 極細で超縦長に引き伸ばされた眩いレーザーライン
                // 進行度（時間）が経つほど光の筋が長く伸びる
                Vector3 startScaleMin = { 0.02f, 1.2f + curveRatio * 2.5f, 0.02f };
                Vector3 startScaleMax = { 0.06f, 2.5f + curveRatio * 3.5f, 0.06f };
                Vector3 endScaleMin = { 0.005f, 0.15f, 0.005f };
                Vector3 endScaleMax = { 0.015f, 0.4f, 0.015f };
                deathGlowParticle_->SetParticleScale(startScaleMin, startScaleMax, endScaleMin, endScaleMax);

                // チャージ完了に近づくほどしきい値を上げ、非常にシャープでシャキッとしたレーザーに見せる
                deathGlowParticle_->SetAlphaReference(0.4f + curveRatio * 0.35f);
                deathGlowParticle_->SetEnableRandomRotation(false); // 進行方向に光の筋を向かせるため回転オフ

                // 超高輝度な黄金の輝き（ブルームによる強い発光）
                Vector4 startColMin = { 5.0f, 4.0f, 0.5f, 1.0f };
                Vector4 startColMax = { 12.0f, 10.0f, 2.0f, 1.0f }; // 超発光
                Vector4 endColMin = { 2.0f, 1.0f, 0.0f, 0.0f };
                Vector4 endColMax = { 5.0f, 2.5f, 0.0f, 0.0f };
                deathGlowParticle_->SetStartColor(startColMin, startColMax);
                deathGlowParticle_->SetEndColor(endColMin, endColMax);
                deathGlowParticle_->SetParticleLife(0.08f, 0.22f); // ごく短い寿命で鋭く明滅する

                deathGlowParticle_->SetEmit(true);
                deathGlowParticle_->Update();
            }

            // 待機中はカメラとパーティクルのみ更新（プレイヤーはその場に留まる）
            weapon_.UpdateParticlesOnly();
            cameraController_.Update(translate_, rotate_, weapon_.GetMissileVibration(), engine_);
            return;
        }

        // 3秒経過後、死亡演出を開始
        if (deathTimer_ == 0) {
            deathBeams_.clear();
            deathBeamDirs_.clear();
            deathBeamDelays_.clear();
            deathBeamOffsets_.clear();

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
        cameraController_.UpdateDeathCamera(deathCameraPos_, translate_, engine_);

        // カメラ更新後にパーティクルのみ更新し、WVP行列を最新化する
        weapon_.UpdateParticlesOnly();

        if (deathGlowParticle_) {
            deathGlowParticle_->SetEmit(false);
            deathGlowParticle_->Update();
        }

        // ★星モデルの座標と回転（ビルボード）を更新
        if (starObj_ && deathTimer_ >= flashTime) {
            // カメラから星へのベクトルを計算して、カメラの方を向かせる（LookAt）
            Vector3 toCamera = {
                deathCameraPos_.x - translate_.x,
                deathCameraPos_.y - translate_.y,
                deathCameraPos_.z - translate_.z
            };
            float lookYaw = std::atan2(toCamera.x, toCamera.z) + 3.14159f;
            float horizontalDist = std::sqrt(toCamera.x * toCamera.x + toCamera.z * toCamera.z);
            float lookPitch = -std::atan2(toCamera.y, horizontalDist);

            starObj_->SetPosition(translate_);
            // XとYの回転でカメラの方向を向きつつ、Zの回転でくるくる回す！
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

            ImGui::Separator();
            ImGui::Text("First Person Mini Figure Settings");
            ImGui::DragFloat3("Mini Pos Offset", &firstPersonMiniPos_.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat3("Mini Scale", &firstPersonMiniScale_.x, 0.001f, 0.001f, 1.0f);
            ImGui::DragFloat("Mini Rot Y Offset", &firstPersonMiniRotY_, 0.01f, -3.14f, 3.14f);

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

            ImGui::Separator();
            ImGui::Text("Damage & Multipliers");
            ImGui::DragInt("Melee Damage", &damageMelee_);
            ImGui::DragFloat("Melee Charge Multi", &damageMeleeChargeMultiplier_, 0.1f);
            ImGui::DragInt("MG Damage", &damageMachineGun_);
            ImGui::DragFloat("MG Charge Multi", &damageMachineGunChargeMultiplier_, 0.1f);
            ImGui::DragInt("Missile Damage", &damageMissile_);
            ImGui::DragFloat("Missile Charge Multi", &damageMissileChargeMultiplier_, 0.1f);

            ImGui::Separator();
            ImGui::Text("Hammer Size");
            ImGui::DragFloat("Base Size", &hammerBaseSize_, 0.1f);
            ImGui::DragFloat("Charge Bonus", &hammerSizeChargeBonus_, 0.1f);
            ImGui::DragFloat("Scale Y Multi", &hammerScaleYMultiplier_, 0.1f);

            ImGui::Separator();
            ImGui::Text("Movement");
            ImGui::DragFloat("Dodge Speed", movement_.GetDodgeSpeedPtr(), 0.1f);
            ImGui::DragFloat("Dodge Normal Multi", movement_.GetDodgeSpeedNormalMultiplierPtr(), 0.05f);

            ImGui::Separator();
            ImGui::Text("Missile Specs");
            ImGui::DragFloat("Turn Speed Normal", weapon_.GetMissileTurnSpeedNormalPtr(), 0.01f);
            ImGui::DragFloat("Turn Speed Charged", weapon_.GetMissileTurnSpeedChargedPtr(), 0.01f);
            ImGui::DragFloat("Spread Base", weapon_.GetMissileSpreadMagnitudeBasePtr(), 0.1f);
            ImGui::DragFloat("Spread Rand", weapon_.GetMissileSpreadMagnitudeRandPtr(), 0.1f);

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

    if (!isCinematicMode_) {
        HandleMovement();
        HandleAttack();
        HandleSkill();
    } else {
        // シネマティック中は攻撃ステートをリセットして振っているハンマー等を消す
        attackState_ = AttackState::kNone;
        attackCollision_.isActive = false;
        attackActiveTimer_ = 0;

        // ボスの座標が設定されていれば、その方向へ自動的に向き直る
        Vector3 toTarget = Math::Subtract(aimPos_, translate_);
        toTarget.y = 0.0f;
        float len = Math::Length(toTarget);
        if (len > 0.1f) {
            float targetYaw = std::atan2(toTarget.x, toTarget.z);
            float diff = targetYaw - rotate_.y;
            
            // 角度の差分を [-PI, PI] の範囲に正規化して最短で回転する
            const float kPi = 3.14159265f;
            const float kTwoPi = 6.2831853f;
            while (diff < -kPi) diff += kTwoPi;
            while (diff > kPi) diff -= kTwoPi;

            rotate_.y += diff * kCinematicRotateSpeed;
        }
    }

    weapon_.Update(translate_, rotate_, cameraController_.GetCameraPitch(), aimPos_, scale_, isKarakuriCharged_);
    cameraController_.Update(translate_, rotate_, weapon_.GetMissileVibration(), engine_);
    status_.UpdateKnockback();

    if (cameraController_.IsFirstPerson()) {
        if (aimingSprite_) {
            // エイムアシストを含めた中心的な射撃方向を計算
            float pitch = cameraController_.GetCameraPitch();
            float cosP = std::cos(pitch);
            float sinP = std::sin(pitch);
            float sinY = std::sin(rotate_.y);
            float cosY = std::cos(rotate_.y);

            Vector3 playerForward = { sinY * cosP, -sinP, cosY * cosP };
            Vector3 blendedForward = playerForward;

            if (isTargetingEnemy_) {
                Vector3 playerCenter = { translate_.x, translate_.y + 1.0f, translate_.z };
                Vector3 aimPos = { targetPos_.x, targetPos_.y + 1.0f, targetPos_.z };
                Vector3 toTarget = { aimPos.x - playerCenter.x, aimPos.y - playerCenter.y, aimPos.z - playerCenter.z };
                float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

                if (dist > 0.001f) {
                    Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
                    float dot = Math::Dot(playerForward, toTargetNorm);

                    // エイムアシストの判定（PlayerWeaponと同様のロジック）
                    float assistThreshold = 0.8f;
                    if (dot > assistThreshold) {
                        float assistRatio = std::pow((dot - assistThreshold) / (1.0f - assistThreshold), 1.5f);
                        // 線形補間
                        blendedForward = playerForward * (1.0f - assistRatio) + toTargetNorm * assistRatio;
                        blendedForward = Math::Normalize(blendedForward);
                    }
                }
            }

            // 射撃方向の先にある一定距離のワールド座標を求める
            Vector3 camPos = engine_->GetCameraManager()->GetActiveCamera()->GetTranslate();
            Vector3 targetWorldPos = {
                camPos.x + blendedForward.x * kAimDistance,
                camPos.y + blendedForward.y * kAimDistance,
                camPos.z + blendedForward.z * kAimDistance
            };

            // ワールド座標からスクリーン座標に変換
            Matrix4x4 viewMat = engine_->GetCameraManager()->GetActiveCamera()->GetViewMatrix();
            Matrix4x4 projMat = engine_->GetCameraManager()->GetActiveCamera()->GetPerspectiveFovMatrix();
            Matrix4x4 viewportMat = engine_->GetCameraManager()->GetActiveCamera()->GetViewportMatrix();

            Matrix4x4 vpMat = Math::Multiply(viewMat, projMat);
            Matrix4x4 vpvMat = Math::Multiply(vpMat, viewportMat);

            Vector3 screenPos = Math::Transform(targetWorldPos, vpvMat);

            // スプライトの座標を更新
            aimingSprite_->SetPosition(screenPos.x, screenPos.y);
            aimingSprite_->Update();
        }
    }
    // プレイヤーとカメラの更新が全て終わった「最新の座標」でUIを更新し、ガタつきを防ぐ
    if (hpBar_) {
        hpBar_->Update(this, cameraController_.IsFirstPerson());
    }

    if (weaponTrail_) {
        weaponTrail_->Update();
    }

    // ★追加: からくりチャージゲージの更新
    if (karakuriGaugeBg_ && karakuriGaugeFill_) {
        // 現在の視点に応じて位置を動的に計算
        float targetX = 440.0f;
        float targetY = 580.0f; // 三人称視点のデフォルト（中央下部）

        if (cameraController_.IsFirstPerson()) {
            // 一人称視点時はHPゲージ（X: 40.0f, Y: ClientHeight - 90.0f）の上に配置
            float clientHeight = engine_ ? static_cast<float>(engine_->GetClientHeight()) : 720.0f;
            targetX = 40.0f;
            targetY = clientHeight - 90.0f - 24.0f; // HPバーの24ピクセル上（余白を含めて綺麗に揃います）
        }

        karakuriGaugeBg_->SetPositionTopLeft(targetX, targetY);
        karakuriGaugeFill_->SetPositionTopLeft(targetX, targetY);

        if (isKarakuriCharged_) {
            // チャージ成功後: 残り時間に応じてゲージを減らす（オレンジ色）
            float ratio = static_cast<float>(karakuriActiveTimer_) / static_cast<float>(kKarakuriActiveTime);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            karakuriGaugeFill_->SetSize(400.0f * ratio, 16.0f);
            karakuriGaugeFill_->SetColor({ 1.0f, 0.5f, 0.0f, 0.9f }); // オレンジ色（効果中）
            karakuriGaugeBg_->Update();
            karakuriGaugeFill_->Update();
        } else if (karakuriChargeTimer_ > 0) {
            // チャージ中: チャージ量に応じてゲージを増やす（黄色）
            float ratio = static_cast<float>(karakuriChargeTimer_) / static_cast<float>(kKarakuriChargeTime);
            if (ratio > 1.0f) ratio = 1.0f;
            karakuriGaugeFill_->SetSize(400.0f * ratio, 16.0f);
            karakuriGaugeFill_->SetColor({ 1.0f, 0.9f, 0.1f, 0.8f }); // 黄色（チャージ中）
            karakuriGaugeBg_->Update();
            karakuriGaugeFill_->Update();
        }
    }

    // ジャスト回避の星エフェクト更新
    if (!status_.IsDead() && starScale_.x > 0.01f) {
        starRotationZ_ += 0.5f; // くるくる回す速度
        starScale_.x *= 0.88f;  // シュッと小さくしていく
        starScale_.y *= 0.88f;
        starScale_.z *= 0.88f;

        if (starObj_) {
            starObj_->SetPosition({ translate_.x, translate_.y + 2.0f, translate_.z });
            // カメラの方を向かせたいが、簡易的に正面に向けるか、今の回転を使用
            starObj_->SetRotate({ rotate_.x, rotate_.y, starRotationZ_ });
            starObj_->SetScale(starScale_);
            starObj_->Update();
        }
    }

    // ★追加: 爆発エフェクトの更新
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->Update();
        }
    }

    // === パーティクル（すっきり立ち上る黄金色シリンダー）の設定・更新 ===
    if (!status_.IsDead()) {
        if (karakuriChargeParticle_) {
            bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;
            if (isCharging && karakuriChargeTimer_ > 0) {
                // === 1. チャージ中の黄金色シリンダー粒子 ===
                float ratio = static_cast<float>(karakuriChargeTimer_) / kKarakuriChargeTime;
                if (ratio > 1.0f) ratio = 1.0f;
                
                // 開始色：すっきりした美しい黄金色から明るい黄色
                Vector4 startColMin = { 1.0f, 0.75f, 0.0f, 1.0f }; // 美しいゴールド
                Vector4 startColMax = { 1.0f, 0.95f, 0.3f, 1.0f }; // 高輝度の明るいイエロー
                // 終了色：上空でスッと消えていくフェードアウト
                Vector4 endColMin = { 1.0f, 0.65f, 0.0f, 0.0f };
                Vector4 endColMax = { 1.0f, 0.85f, 0.1f, 0.0f };
                
                karakuriChargeParticle_->SetStartColor(startColMin, startColMax);
                karakuriChargeParticle_->SetEndColor(endColMin, endColMax);
                karakuriChargeParticle_->SetParticleLife(0.4f, 0.75f); // 寿命
                
                // gradationLine.png に適した、縦長に引き伸ばされた極細の光の縦筋
                Vector3 startScaleMin = { 0.04f, 1.2f, 0.04f };
                Vector3 startScaleMax = { 0.10f, 2.4f, 0.10f };
                Vector3 endScaleMin = { 0.005f, 0.2f, 0.005f };
                Vector3 endScaleMax = { 0.02f, 0.5f, 0.02f };
                karakuriChargeParticle_->SetParticleScale(startScaleMin, startScaleMax, endScaleMin, endScaleMax);
                
                // チャージが溜まるにつれて、しきい値を徐々に高くして光の筋を細くシャープにする
                float alphaRef = 0.05f + ratio * 0.75f;
                karakuriChargeParticle_->SetAlphaReference(alphaRef);
                
                // 適度な上昇力と少なめの空気抵抗
                karakuriChargeParticle_->SetGravity(-15.0f - ratio * 10.0f);
                karakuriChargeParticle_->SetDamping(0.02f);
                
                // すっきりまっすぐ立ち上らせるためにジッターを極めて小さく
                karakuriChargeParticle_->SetJitter(0.08f);
                karakuriChargeParticle_->SetEnableRandomRotation(false); // 縦筋の方向を保つため回転をオフ
                
                // 円柱エミッターをプレイヤーの周囲に配置
                Vector3 emitPos = translate_;
                emitPos.y += 0.0f; // 足元から
                
                uint32_t count = 15 + static_cast<uint32_t>(ratio * 25.0f); // 溜まるほど高密度に
                float freq = 0.03f - ratio * 0.015f;
                
                karakuriChargeParticle_->SetCylinderEmitter(emitPos, { 0.0f, 1.0f, 0.0f }, 1.3f, 2.0f, count, freq);
                karakuriChargeParticle_->SetEmit(true);
                karakuriChargeParticle_->Update();
            } else if (isKarakuriCharged_) {
                // === 2. チャージ完了時（スーパーサイヤ人状態）の常時黄金色シリンダー粒子 ===
                Vector4 startColMin = { 1.0f, 0.80f, 0.05f, 1.0f }; // まばゆいゴールド
                Vector4 startColMax = { 1.0f, 0.98f, 0.4f, 1.0f }; // 鮮やかなイエロー
                Vector4 endColMin = { 1.0f, 0.70f, 0.0f, 0.0f };
                Vector4 endColMax = { 1.0f, 0.90f, 0.1f, 0.0f };
                
                karakuriChargeParticle_->SetStartColor(startColMin, startColMax);
                karakuriChargeParticle_->SetEndColor(endColMin, endColMax);
                karakuriChargeParticle_->SetParticleLife(0.35f, 0.7f);
                
                // 縦長に引き伸ばされた極細の光の縦筋
                Vector3 startScaleMin = { 0.03f, 1.0f, 0.03f };
                Vector3 startScaleMax = { 0.08f, 2.0f, 0.08f };
                Vector3 endScaleMin = { 0.005f, 0.15f, 0.005f };
                Vector3 endScaleMax = { 0.015f, 0.4f, 0.015f };
                karakuriChargeParticle_->SetParticleScale(startScaleMin, startScaleMax, endScaleMin, endScaleMax);
                
                // 完了状態では高いしきい値で非常にシャープでシャキッとした光の筋を維持
                karakuriChargeParticle_->SetAlphaReference(0.7f);
                
                karakuriChargeParticle_->SetGravity(-18.0f);
                karakuriChargeParticle_->SetDamping(0.02f);
                
                karakuriChargeParticle_->SetJitter(0.08f);
                karakuriChargeParticle_->SetEnableRandomRotation(false);
                
                Vector3 emitPos = translate_;
                emitPos.y += 0.0f;
                
                uint32_t count = 30; // 高密度
                float freq = 0.02f;
                
                karakuriChargeParticle_->SetCylinderEmitter(emitPos, { 0.0f, 1.0f, 0.0f }, 1.3f, 2.0f, count, freq);
                karakuriChargeParticle_->SetEmit(true);
                karakuriChargeParticle_->Update();
            } else {
                // チャージもされておらず、チャージキーも押していない場合はエミッターを停止
                karakuriChargeParticle_->SetAlphaReference(0.0f);
                karakuriChargeParticle_->SetEmit(false);
                karakuriChargeParticle_->Update();
            }
        }
    } else {
        if (karakuriChargeParticle_) {
            karakuriChargeParticle_->SetAlphaReference(0.0f);
            karakuriChargeParticle_->SetEmit(false);
            karakuriChargeParticle_->Update();
        }
    }

    // リングパーティクルの更新（毎フレーム実行し、放出済みの粒子をアニメーションさせる）
    if (karakuriRingParticle_) {
        // エミッター自動放出は常にオフ
        karakuriRingParticle_->SetEmit(false);
        karakuriRingParticle_->Update();
    }

    // 死亡光線パーティクルの更新（生存中は放出せず、残存粒子をアニメーション更新）
    if (deathGlowParticle_) {
        deathGlowParticle_->SetEmit(false);
        deathGlowParticle_->Update();
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
                addSphereLines(attackCollision_.center, attackCollision_.radius * 1.05f + 0.2f, greenColor);
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

void Player::Draw3DUI(Enemy* enemy, bool isUI, bool isPaused) {
    if (!status_.IsDead()) {
        if (!cameraController_.IsFirstPerson()) {
            if (hpBar_ && !isPaused) {
                hpBar_->Draw3D(isUI);
            }
        } else {
            // ボスや部位のHPバー（3D）は1人称視点のみ表示
            if (enemy) {
                enemy->Draw3DUI(engine_, false); // Zバッファを使った遮蔽計算を行うため Standard3DQueue (false) に送る
            }
        }
    }
}

void Player::Draw2DUI(Enemy* enemy) {
    if (!status_.IsDead()) {
        // 1. 各視点別のUIを描画
        if (cameraController_.IsFirstPerson()) {
            if (maskSprite_) maskSprite_->Draw();
            if (aimingSprite_) aimingSprite_->Draw();
            if (hpBar_) hpBar_->Draw2D();

            // ボスのHPバー（2D）は1人称視点のみ表示
            if (enemy) {
                enemy->Draw2DUI(engine_, true);
            }
        } else {
            // 三人称視点時：ボスのHPバーは表示せず、警告演出スプライト（注意マークと矢印）のみ表示する
            if (enemy) {
                enemy->Draw2DUI(engine_, false);
            }
        }

        // 2. 最も手前に描画されるべき「からくりチャージゲージ」を最後に描画
        // チャージ中 or チャージ成功後（効果時間中）はゲージを表示する
        bool showKarakuriGauge = isKarakuriCharged_ || karakuriChargeTimer_ > 0;
        if (showKarakuriGauge && karakuriGaugeBg_ && karakuriGaugeFill_) {
            karakuriGaugeBg_->Draw();
            karakuriGaugeFill_->Draw();
        }
    }
}

void Player::Draw() {
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

            // からくりチャージ中（Eキー長押し中）のシェイク演出
            if (karakuriChargeTimer_ > 0 && !isKarakuriCharged_) {
                float shakeScale = static_cast<float>(karakuriChargeTimer_) / kKarakuriChargeTime;
                drawPos.x += ((std::rand() % 100) / 100.0f - 0.5f) * 0.2f * shakeScale;
                drawPos.y += ((std::rand() % 100) / 100.0f - 0.5f) * 0.2f * shakeScale;
                drawPos.z += ((std::rand() % 100) / 100.0f - 0.5f) * 0.2f * shakeScale;
            }
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
        } else if (cameraController_.IsFirstPerson()) {
            // 一人称視点時はHPゲージの上に小さなプレイヤーモデルをフィギュア風に描画
            if (!isBlinking) {
                Camera* camera = engine_ ? engine_->GetCameraManager()->GetActiveCamera() : nullptr;
                if (camera) {
                    // 元のパラメータを退避
                    Vector3 origPos = obj_->GetPosition();
                    Vector3 origRot = obj_->GetRotate();
                    Vector3 origScale = obj_->GetScale();

                    // カメラのローカル空間での左下前方の位置（HPゲージのすぐ上）
                    Vector3 localOffset = firstPersonMiniPos_; 
                    Matrix4x4 camWorld = camera->GetWorldMatrix();
                    Vector3 drawPos = Math::Transform(localOffset, camWorld);

                    // 少し斜めを向かせる回転（カメラ向き + Y軸回転補正）
                    Vector3 drawRot = camera->GetRotate();
                    drawRot.y += firstPersonMiniRotY_; // 少し斜めを向かせて立体感を持たせる

                    obj_->SetPosition(drawPos);
                    obj_->SetRotate(drawRot);
                    obj_->SetScale(firstPersonMiniScale_); // ミニチュアサイズ
                    obj_->Update();

                    obj_->Draw();

                    // パラメータを元に戻す
                    obj_->SetPosition(origPos);
                    obj_->SetRotate(origRot);
                    obj_->SetScale(origScale);
                    obj_->Update();
                }
            }
        } else if (!isBlinking) {
            obj_->Draw();
        }
    }

    // ★追加: 星（plane.obj）の描画
    if (starObj_ && starScale_.x > 0.01f) {
        if (status_.IsDead() && deathTimer_ >= kDeathAnimationDuration - 40) {
            starObj_->Draw();
        } else if (!status_.IsDead()) {
            starObj_->Draw(); // ジャスト回避エフェクト
        }
    }

    if (attackObj_ && attackState_ != AttackState::kNone && !status_.IsDead() && cameraController_.IsCameraControlEnabled()) {
        attackObj_->Draw();
    }

    if (weaponTrail_) {
        weaponTrail_->SyncBeforeDraw();
        weaponTrail_->Draw();
    }

    if (isTargetingEnemy_ && targetMarkerObj_ && !status_.IsDead()) {
        // targetMarkerObj_->Draw();
    }

    weapon_.Draw(translate_, rotate_, cameraController_.GetCameraPitch(), aimPos_, static_cast<int>(cameraController_.GetViewMode()), isBlinking, status_.IsDead());

    // ★追加: 爆発エフェクトの描画
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->SyncBeforeDraw();
            effect->Draw();
        }
    }

    // ★追加: からくりチャージエフェクトの描画
    if (!status_.IsDead()) {
        bool isCharging = input_->IsKeyDown('E') && !isKarakuriCharged_;
        
        if (isKarakuriCharged_ || (isCharging && karakuriChargeTimer_ > 0)) {
            // チャージ中および完了時（スーパーサイヤ人状態）の黄金色シリンダーパーティクル描画
            if (karakuriChargeParticle_) {
                karakuriChargeParticle_->SyncBeforeDraw();
                karakuriChargeParticle_->Draw();
            }
        }

        // チャージしきったときの足元リングエフェクト（衝撃波）の描画
        if (karakuriRingParticle_) {
            karakuriRingParticle_->SyncBeforeDraw();
            karakuriRingParticle_->Draw();
        }
    }

    // 死亡待機中の自爆前光線エフェクトの描画
    if (deathGlowParticle_) {
        deathGlowParticle_->SyncBeforeDraw();
        deathGlowParticle_->Draw();
    }

    // ★追加: 死亡演出前の自爆電撃ビームの描画
    if (status_.IsDead() && deathWaitTimer_ < kDeathWaitTime) {
        for (size_t i = 0; i < deathBeams_.size(); ++i) {
            if (deathBeams_[i]->IsAttackActive()) {
                deathBeams_[i]->Draw(engine_);
            }
        }
    }


    // 照準とマスクは Draw2DUI で描画するように変更

#ifdef USE_IMGUI
    if (lineOBB_ && isDebugDrawOBB_ && engine_) {
        lineOBB_->Draw();
    }
#endif
}

void Player::DrawParticles() {
    weapon_.DrawParticles(engine_);
}

bool Player::ApplyDamage(int damage) {
    // ジャスト回避の判定
    if (movement_.IsJustEvasionWindow()) {
        // ジャスト回避成功！
        // 無敵時間を大幅に付与（180フレーム = 3秒間）して後続の攻撃を回避
        status_.SetInvincibleTimer(180); 

        // ジャスト回避成功時のキラン☆演出（死亡時の星モデルを一時的に流用）
        if (starObj_) {
            starScale_ = { 4.0f, 4.0f, 4.0f }; // 星を出す
            starRotationZ_ = 0.0f;
            // deathTimer_等に依存せず星を描画するため、Drawメソッドでの描画条件を追加する必要がありますが、
            // 現在は無敵付与による点滅で回避成功が分かります。
        }
        return false; // ダメージは受けない
    }

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

            /**
             * @brief 攻撃開始時のモデル座標・回転の初期化
             * 
             * @details
             * 攻撃開始フレームで attackObj_ の座標を直ちにプレイヤー位置へ更新する。
             * これを行わない場合、次の Draw() 呼び出し時に1フレームだけ
             * 「前回攻撃が終了した座標」にモデルが描画される（残像が残る）現象が発生する。
             */
            float currentAngle = rotate_.y + kHammerAngleOffset;
            float sinA = std::sin(currentAngle);
            float cosA = std::cos(currentAngle);

            Vector3 hammerPos;
            hammerPos.x = translate_.x + sinA * kSwingBaseRadius;
            hammerPos.y = translate_.y + kHammerBaseHeight;
            hammerPos.z = translate_.z + cosA * kSwingBaseRadius;

            if (attackObj_) {
                attackObj_->SetPosition(hammerPos + weapon_.GetMissileVibration());
                Vector3 swingRot = rotate_;
                swingRot.y = currentAngle;
                swingRot.x = kHammerRotX;
                attackObj_->SetRotate(swingRot);
                attackObj_->SetScale({ 1.0f, 1.0f, 1.0f });
                attackObj_->Update();
            }
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
                float baseScale = 1.0f + (chargeRate * hammerSizeChargeBonus_);
                Vector3 hammerScale = { baseScale, baseScale, baseScale };
                attackObj_->SetScale(hammerScale);
                attackObj_->Update();
            }
        } else {
            attackState_ = AttackState::kAttacking;
            if (seHammer_) seHammer_->Play();
            hasPlayedHammerHitThisAttack_ = false;
            attackActiveTimer_ = kAttackDuration;
            attackCollision_.isActive = true;
            currentChargeRate_ = static_cast<float>(chargeTimer_) / kMaxChargeTime;
            if (currentChargeRate_ > 1.0f) currentChargeRate_ = 1.0f;

            float baseScale = 1.0f + (currentChargeRate_ * hammerSizeChargeBonus_);
            Vector3 hammerScale = { baseScale, baseScale, baseScale };
            attackCollision_.radius = baseScale * 1.5f; // モデル(直径約3m)に合わせて半径1.5m

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
                float baseScale = 1.0f + (currentChargeRate_ * hammerSizeChargeBonus_);
                Vector3 hammerScale = { baseScale, baseScale, baseScale };
                attackObj_->SetScale(hammerScale);
                attackObj_->Update();
            }

            if (weaponTrail_) {
                Vector3 tipPos = attackCollision_.center + weapon_.GetMissileVibration();
                // プレイヤーの中心からtipPosへのベクトルを計算し、軌跡を扇形ではなく「帯（リボン）」にする
                // 根本（basePos）をハンマーの中心とプレイヤーの中心の間に設定（例えば60%の位置）
                Vector3 basePos;
                basePos.x = Lerp(translate_.x, tipPos.x, 0.6f);
                basePos.y = Lerp(translate_.y + kHammerBaseHeight, tipPos.y, 0.6f);
                basePos.z = Lerp(translate_.z, tipPos.z, 0.6f);

                weaponTrail_->AddPoint(basePos, tipPos);
            }

            attackActiveTimer_--;
            if (attackActiveTimer_ <= 0) {
                attackCollision_.isActive = false;
                attackState_ = AttackState::kNone;
                if (weaponTrail_) weaponTrail_->StopTrail();
            }
        }
        break;
    }
}

void Player::HandleSkill() {
    if (skillDurationTimer_ > 0) {
        skillDurationTimer_--;
        if (skillDurationTimer_ <= 0 && !isMachineGunSkillActive_) {
            // ミサイルスキルのみクールダウンを設定
            skillCooldownTimer_ = kSkillCooldownTime;
        }
    } else if (skillCooldownTimer_ > 0) {
        skillCooldownTimer_--;
    }

    if (cooldownWarningTimer_ > 0) {
        cooldownWarningTimer_--;
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
            if (karakuriChargeTimer_ == 0 && seKarakuri_) {
                seKarakuri_->Play(true);
            }
            karakuriChargeTimer_++;
            if (karakuriChargeTimer_ >= kKarakuriChargeTime) {
                isKarakuriCharged_ = true;
                if (seKarakuri_) seKarakuri_->Stop();
                karakuriChargeTimer_ = 0;
                karakuriActiveTimer_ = kKarakuriActiveTime;

                // ★追加: チャージしきったときの足元リングエフェクト（衝撃波）バースト放出
                if (karakuriRingParticle_) {
                    // 足元からXZ平面上にフワッと広げる
                    Vector3 emitPos = translate_;
                    emitPos.y += 0.1f; // 地面から少しだけ浮いた高さ

                    // リングエミッターの設定 (放出位置, 半径, 厚み, 放出数, 放出頻度)
                    // 放出頻度を0.0fに設定して自動毎フレーム放出を防ぎ、Emit()によるバーストのみにする
                    karakuriRingParticle_->SetRingEmitter(emitPos, 0.4f, 0.1f, 180, 0.0f);
                    
                    // 速度の方向ベクトルを {0,0,0} にリセットすることで
                    // 半径方向（水平方向）に均等に綺麗に広がるようにする
                    karakuriRingParticle_->SetDirection({ 0.0f, 0.0f, 0.0f });
                    karakuriRingParticle_->SetVelocity(16.0f); // 広がる初速
                    karakuriRingParticle_->SetJitter(0.0f);
                    karakuriRingParticle_->SetGravity(0.0f);
                    karakuriRingParticle_->SetDamping(0.05f); // 摩擦で滑らかに減速する
                    karakuriRingParticle_->SetParticleLife(0.55f, 0.75f); // 寿命

                    // 粒子のスケール：最初は小さい点、広がりながら極小になって消えていく
                    Vector3 ringStartScaleMin = { 0.12f, 0.12f, 0.12f };
                    Vector3 ringStartScaleMax = { 0.28f, 0.28f, 0.28f };
                    Vector3 ringEndScaleMin = { 0.0f, 0.0f, 0.0f };
                    Vector3 ringEndScaleMax = { 0.0f, 0.0f, 0.0f };
                    karakuriRingParticle_->SetParticleScale(ringStartScaleMin, ringStartScaleMax, ringEndScaleMin, ringEndScaleMax);

                    // 黄金色の衝撃波カラー（ゴールドからイエロー）
                    Vector4 ringStartColMin = { 1.0f, 0.75f, 0.0f, 1.0f };
                    Vector4 ringStartColMax = { 1.0f, 0.95f, 0.2f, 1.0f };
                    Vector4 ringEndColMin = { 1.0f, 0.65f, 0.0f, 0.0f };
                    Vector4 ringEndColMax = { 1.0f, 0.85f, 0.0f, 0.0f };
                    karakuriRingParticle_->SetStartColor(ringStartColMin, ringStartColMax);
                    karakuriRingParticle_->SetEndColor(ringEndColMin, ringEndColMax);

                    // 180個の粒子を瞬間バースト放出！
                    karakuriRingParticle_->Emit(180);
                }
            }
        }
    } else {
        if (!isKarakuriCharged_) {
            if (karakuriChargeTimer_ > 0 && seKarakuri_) {
                seKarakuri_->Stop();
            }
            karakuriChargeTimer_ = 0;
        }
    }

#ifdef USE_IMGUI
    if (!engine_->IsCursorLocked() && ImGui::GetIO().WantCaptureMouse) return;
#endif

    if (!cameraController_.IsCameraControlEnabled()) return;

    if (input_->IsMouseButtonPressed(Mouse::Button::Right)) {
        if (!isKarakuriCharged_) {
            // ========== 機関銃スキル（クールダウンなし・弾薬ベース） ==========
            if (weapon_.IsMachineGunFiring()) {
                // 発射中 → 右クリックで即停止（クールダウンなし・即再発射可能）
                weapon_.StopMachineGunSkill();
                skillDurationTimer_ = 0;
                isMachineGunSkillActive_ = false;
            } else if (weapon_.GetMachineGunAmmo() >= kMinAmmoToRestart) {
                // 停止中 かつ 残弾が最低数以上 → 発射開始
                weapon_.StartMachineGunSkill();
                skillDurationTimer_ = kMachineGunSkillDuration;
                isMachineGunSkillActive_ = true;
            }
            // 残弾が最低数未満 → 何もしない（回復待ち）
        } else {
            // ========== ミサイルスキル（従来のクールダウン制御） ==========
            if (skillDurationTimer_ <= 0 && skillCooldownTimer_ <= 0) {
                int fireCount = isTargetingEnemy_ ? 2 : 1;
                for (int i = 0; i < fireCount; ++i) {
                    weapon_.FireMissileSkill(translate_, rotate_, targetPos_);
                }
                skillDurationTimer_ = kMissileSkillDuration;
                isMachineGunSkillActive_ = false;
            } else if (skillDurationTimer_ <= 0 && skillCooldownTimer_ > 0) {
                cooldownWarningTimer_ = 60;
                if (seCooldown_) seCooldown_->Play();
            }
        }
    }
}

void Player::HitAndKnockback(Enemy* enemy) {
    status_.HitAndKnockback(enemy, translate_);
}

void Player::OnMeleeHit() {
    if (!hasPlayedHammerHitThisAttack_) {
        if (seHammer_) seHammer_->Stop();
        if (seHammerHit_) seHammerHit_->Play();
        hasPlayedHammerHitThisAttack_ = true;
    }
}

void Player::PlayExplosion(const Vector3& position, float scale) {
    for (auto& effect : explosionEffects_) {
        if (!effect->IsActive()) {
            effect->Play(position, { 0.0f, 0.0f, 0.0f }, { scale, scale, scale });
            if (seMissileHit_) seMissileHit_->Play();
            break; // 同時に1つの着弾で1つのみ再生
        }
    }
}