#include "PlayerWeapon.h"
#include "Engine/Core/Math/Math.h"
#include "Renderer/Particle/ParticleSystem.h"
#include "Renderer/Region/ModelRegion.h"
#include <cmath>
#include <cstdlib>

void PlayerWeapon::Initialize() {

    // --- 機関銃モデルの初期化 ---
    machineGunObjLeft_ = std::make_unique<ObjClass>();
    machineGunObjLeft_->Initialize("enemy/body.obj");

    machineGunObjRight_ = std::make_unique<ObjClass>();
    machineGunObjRight_->Initialize("enemy/body.obj");

    // --- 機関銃の弾モデルの初期化 ---
    bulletRegion_ = std::make_unique<ModelRegion>();
    bulletRegion_->Initialize("enemy/body.obj");
    bulletRegion_->SetCastShadows(false);

    for (int i = 0; i < kMaxBullets; ++i) {
        bullets_[i].isActive = false;
    }
    machineGunActiveTimer_ = 0;
    machineGunFireTimer_ = 0;

    // --- 銃口の煙パーティクルの初期化 ---
    muzzleSmokeLeft_ = std::make_unique<ParticleSystem>();
    muzzleSmokeLeft_->Initialize("resources/circle.png", ParticleType::kMuzzleSmoke);
    muzzleSmokeRight_ = std::make_unique<ParticleSystem>();
    muzzleSmokeRight_->Initialize("resources/circle.png", ParticleType::kMuzzleSmoke);

    // --- マズルフラッシュパーティクルの初期化 ---
    muzzleFlashLeft_ = std::make_unique<ParticleSystem>();
    muzzleFlashLeft_->Initialize("resources/whiteTexture.png", ParticleType::kMuzzleFlash, PrimitiveType::Circle);
    muzzleFlashLeft_->SetBlend(BlendMode::kBlendModeNormal);
    muzzleFlashRight_ = std::make_unique<ParticleSystem>();
    muzzleFlashRight_->Initialize("resources/whiteTexture.png", ParticleType::kMuzzleFlash, PrimitiveType::Circle);
    muzzleFlashRight_->SetBlend(BlendMode::kBlendModeNormal);

    // --- 加算合成マズルフラッシュの初期化 ---
    muzzleFlashAddLeft_ = std::make_unique<ParticleSystem>();
    muzzleFlashAddLeft_->Initialize("resources/circle.png", ParticleType::kMuzzleFlash);
    muzzleFlashAddLeft_->SetBlend(BlendMode::kBlendModeAdd);
    muzzleFlashAddRight_ = std::make_unique<ParticleSystem>();
    muzzleFlashAddRight_->Initialize("resources/circle.png", ParticleType::kMuzzleFlash);
    muzzleFlashAddRight_->SetBlend(BlendMode::kBlendModeAdd);

    missileFire_ = std::make_unique<ParticleSystem>();
    missileFire_->Initialize("resources/circle.png", ParticleType::kMissileFire);
    missileFire_->SetCullingEnabled(false);
    missileFire_->SetBlend(BlendMode::kBlendModeAdd);

    missileSmoke_ = std::make_unique<ParticleSystem>();
    missileSmoke_->Initialize("resources/circle.png", ParticleType::kMissileSmoke);
    missileSmoke_->SetCullingEnabled(false);

    bulletTrail_ = std::make_unique<ParticleSystem>();
    bulletTrail_->Initialize("resources/circle.png", ParticleType::kBulletTrail);
    bulletTrail_->SetBlend(BlendMode::kBlendModeAdd);
    bulletTrail_->SetCullingEnabled(false);

    ejectionMistLeft_ = std::make_unique<ParticleSystem>();
    ejectionMistLeft_->Initialize("resources/circle.png", ParticleType::kEjectionMist);
    ejectionMistLeft_->SetBlend(BlendMode::kBlendModeAdd);

    ejectionMistRight_ = std::make_unique<ParticleSystem>();
    ejectionMistRight_->Initialize("resources/circle.png", ParticleType::kEjectionMist);
    ejectionMistRight_->SetBlend(BlendMode::kBlendModeAdd);

    // --- 薬莢モデルの初期化 ---
    cartridgeRegion_ = std::make_unique<ModelRegion>();
    cartridgeRegion_->Initialize("enemy/body.obj");
    cartridgeRegion_->SetCastShadows(false);

    for (int i = 0; i < kMaxCartridges; ++i) {
        cartridges_[i].isActive = false;
    }

    // --- ミサイルモデルとデータの初期化 ---
    missileRegion_ = std::make_unique<ModelRegion>();
    missileRegion_->Initialize("enemy/body.obj");
    missileRegion_->SetCastShadows(false);

    for (int i = 0; i < kMaxMissiles; ++i) {
        missiles_[i].isActive = false;
    }

    machineGunVibration_ = { 0.0f, 0.0f, 0.0f };
    missileVibration_ = { 0.0f, 0.0f, 0.0f };
    missileVibrationTimer_ = 0;
}

void PlayerWeapon::Update(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos, const Vector3& playerScale, bool isKarakuriCharged) {
    // 振動（シェイク）の更新
    if (machineGunActiveTimer_ <= 0) {
        machineGunVibration_.x *= 0.8f;
        machineGunVibration_.y *= 0.8f;
        machineGunVibration_.z *= 0.8f;
    }

    if (missileVibrationTimer_ > 0) {
        missileVibrationTimer_--;
        missileVibration_.x = ((std::rand() % 100) / 100.0f - 0.5f) * missileVibrationScale_ * 2.0f;
        missileVibration_.y = ((std::rand() % 100) / 100.0f - 0.5f) * missileVibrationScale_ * 2.0f;
        missileVibration_.z = ((std::rand() % 100) / 100.0f - 0.5f) * missileVibrationScale_ * 2.0f;
    } else if (missileVibration_.x != 0.0f || missileVibration_.y != 0.0f || missileVibration_.z != 0.0f) {
        missileVibration_.x *= 0.8f;
        missileVibration_.y *= 0.8f;
        missileVibration_.z *= 0.8f;
    }

    // 各武器の更新
    UpdateMissile(targetPos, playerScale, isKarakuriCharged);
    UpdateMachineGun(playerTranslate, playerRotate, cameraPitch, targetPos);
    UpdateCartridges();

    // パーティクルの更新
    if (muzzleSmokeLeft_) muzzleSmokeLeft_->Update();
    if (muzzleSmokeRight_) muzzleSmokeRight_->Update();
    if (muzzleFlashLeft_) muzzleFlashLeft_->Update();
    if (muzzleFlashRight_) muzzleFlashRight_->Update();
    if (muzzleFlashAddLeft_) muzzleFlashAddLeft_->Update();
    if (muzzleFlashAddRight_) muzzleFlashAddRight_->Update();
    if (missileFire_) missileFire_->Update();
    if (missileSmoke_) missileSmoke_->Update();
    if (bulletTrail_) bulletTrail_->Update();
    if (ejectionMistLeft_) ejectionMistLeft_->Update();
    if (ejectionMistRight_) ejectionMistRight_->Update();
}

void PlayerWeapon::UpdateParticlesOnly() {
    // 薬莢の更新
    UpdateCartridges();

    // 既存の弾の更新
    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive) {
            bullets_[i].position.x += bullets_[i].velocity.x;
            bullets_[i].position.y += bullets_[i].velocity.y;
            bullets_[i].position.z += bullets_[i].velocity.z;

            if (bulletTrail_) bulletTrail_->PlayHitEffect(bullets_[i].position, 2);

            bullets_[i].timer--;
            if (bullets_[i].timer <= 0) {
                bullets_[i].isActive = false;
            }
        }
    }

    // ミサイルの慣性移動とパーティクル更新
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive) {
            missiles_[i].position.x += missiles_[i].velocity.x;
            missiles_[i].position.y += missiles_[i].velocity.y;
            missiles_[i].position.z += missiles_[i].velocity.z;

            // 地面に潜らないようにする
            if (missiles_[i].position.y < 0.1f) {
                missiles_[i].position.y = 0.1f;
                if (missiles_[i].velocity.y < 0.0f) {
                    missiles_[i].velocity.y = 0.0f;
                }
            }

            float missileHalfLength = 2.2f; // 描画スケール(Z=0.8)に基づく固定値 (尻尾よりわずかに内側)
            float vx = missiles_[i].velocity.x;
            float vy = missiles_[i].velocity.y;
            float vz = missiles_[i].velocity.z;
            float speed = std::sqrt(vx * vx + vy * vy + vz * vz);
            Vector3 tailPos = missiles_[i].position;

            if (speed > 0.001f) {
                tailPos.x -= (vx / speed) * missileHalfLength;
                tailPos.y -= (vy / speed) * missileHalfLength;
                tailPos.z -= (vz / speed) * missileHalfLength;
            }

            if (missileFire_) missileFire_->PlayHitEffect(tailPos, 3);
            if (missileSmoke_) missileSmoke_->PlayHitEffect(tailPos, 2);

            missiles_[i].timer--;
            if (missiles_[i].timer <= 0) missiles_[i].isActive = false;
        }
    }

    // パーティクルの更新
    if (muzzleSmokeLeft_) muzzleSmokeLeft_->Update();
    if (muzzleSmokeRight_) muzzleSmokeRight_->Update();
    if (muzzleFlashLeft_) muzzleFlashLeft_->Update();
    if (muzzleFlashRight_) muzzleFlashRight_->Update();
    if (muzzleFlashAddLeft_) muzzleFlashAddLeft_->Update();
    if (muzzleFlashAddRight_) muzzleFlashAddRight_->Update();
    if (missileFire_) missileFire_->Update();
    if (missileSmoke_) missileSmoke_->Update();
    if (bulletTrail_) bulletTrail_->Update();
    if (ejectionMistLeft_) ejectionMistLeft_->Update();
    if (ejectionMistRight_) ejectionMistRight_->Update();
}

void PlayerWeapon::Draw(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos, int viewMode, bool isBlinking, bool isDead) {
    if (machineGunObjLeft_ && machineGunObjRight_ && !isDead) {
        float sinY = std::sin(playerRotate.y);
        float cosY = std::cos(playerRotate.y);
        float rightX = cosY;
        float rightZ = -sinY;

        Vector3 leftShoulder = { playerTranslate.x - rightX * 0.7f, playerTranslate.y + 1.0f, playerTranslate.z - rightZ * 0.7f };
        Vector3 rightShoulder = { playerTranslate.x + rightX * 0.7f, playerTranslate.y + 1.0f, playerTranslate.z + rightZ * 0.7f };

        Vector3 playerCenter = { playerTranslate.x, playerTranslate.y + 1.0f, playerTranslate.z };
        Vector3 aimPos = { targetPos.x, targetPos.y + 1.0f, targetPos.z };
        Vector3 toTarget = { aimPos.x - playerCenter.x, aimPos.y - playerCenter.y, aimPos.z - playerCenter.z };

        Vector3 rot = { 0.0f, 0.0f, 0.0f };
        float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

        // ★修正: 照準に近いほど吸い寄せる（エイムアシスト）
        float cosP = std::cos(cameraPitch);
        float sinP = std::sin(cameraPitch);
        Vector3 playerForward = { std::sin(playerRotate.y) * cosP, -sinP, std::cos(playerRotate.y) * cosP };

        Vector3 blendedForward = playerForward;

        if (dist > 0.001f) {
            Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
            float dot = playerForward.x * toTargetNorm.x + playerForward.y * toTargetNorm.y + playerForward.z * toTargetNorm.z;

            float assistThreshold = 0.8f;
            if (dot > assistThreshold) {
                float assistRatio = (dot - assistThreshold) / (1.0f - assistThreshold);
                assistRatio = std::pow(assistRatio, 1.5f);

                blendedForward.x = playerForward.x * (1.0f - assistRatio) + toTargetNorm.x * assistRatio;
                blendedForward.y = playerForward.y * (1.0f - assistRatio) + toTargetNorm.y * assistRatio;
                blendedForward.z = playerForward.z * (1.0f - assistRatio) + toTargetNorm.z * assistRatio;

                float fLen = std::sqrt(blendedForward.x * blendedForward.x + blendedForward.y * blendedForward.y + blendedForward.z * blendedForward.z);
                blendedForward.x /= fLen;
                blendedForward.y /= fLen;
                blendedForward.z /= fLen;
            }
        }

        // ブレンドされたベクトルから回転角(rot)を再計算する
        rot.y = std::atan2(blendedForward.x, blendedForward.z);
        float xzLen = std::sqrt(blendedForward.x * blendedForward.x + blendedForward.z * blendedForward.z);
        rot.x = std::atan2(-blendedForward.y, xzLen);

        // 機関銃モデルの位置に機関銃振動を反映
        machineGunObjLeft_->SetPosition(leftShoulder + machineGunVibration_);
        machineGunObjLeft_->SetRotate(rot);
        machineGunObjLeft_->SetScale(kMachineGunScale);
        machineGunObjLeft_->Update();

        machineGunObjRight_->SetPosition(rightShoulder + machineGunVibration_);
        machineGunObjRight_->SetRotate(rot);
        machineGunObjRight_->SetScale(kMachineGunScale);
        machineGunObjRight_->Update();

        // viewMode == 0 が kFirstPerson の想定
        if (viewMode != 0 && !isBlinking) {
            machineGunObjLeft_->Draw();
            machineGunObjRight_->Draw();
        }

        float muzzleOffsetSize = (kMachineGunModelSize.z * 0.5f) * kMachineGunScale.z;
        float cosRotX = std::cos(rot.x);
        Vector3 forward = { std::sin(rot.y) * cosRotX, -std::sin(rot.x), std::cos(rot.y) * cosRotX };
        Vector3 muzzleLeft = { leftShoulder.x + forward.x * muzzleOffsetSize, leftShoulder.y + forward.y * muzzleOffsetSize, leftShoulder.z + forward.z * muzzleOffsetSize };
        Vector3 muzzleRight = { rightShoulder.x + forward.x * muzzleOffsetSize, rightShoulder.y + forward.y * muzzleOffsetSize, rightShoulder.z + forward.z * muzzleOffsetSize };

        if (muzzleSmokeLeft_) muzzleSmokeLeft_->SetEmitterPosition(leftShoulder);
        if (muzzleSmokeRight_) muzzleSmokeRight_->SetEmitterPosition(rightShoulder);
        if (muzzleFlashLeft_) muzzleFlashLeft_->SetEmitterPosition(muzzleLeft);
        if (muzzleFlashRight_) muzzleFlashRight_->SetEmitterPosition(muzzleRight);
        if (muzzleFlashAddLeft_) muzzleFlashAddLeft_->SetEmitterPosition(muzzleLeft);
        if (muzzleFlashAddRight_) muzzleFlashAddRight_->SetEmitterPosition(muzzleRight);
    }

    // 薬莢の描画
    if (cartridgeRegion_) {
        cartridgeRegion_->ClearInstances();
        for (int i = 0; i < kMaxCartridges; ++i) {
            if (cartridges_[i].isActive) {
                Transform tf;
                tf.translate = cartridges_[i].position;
                tf.rotate = cartridges_[i].rotation;
                tf.scale = { 0.02f, 0.02f, 0.05f };
                cartridgeRegion_->AddInstance(tf, { 0.8f, 0.6f, 0.1f, 1.0f });
            }
        }
        cartridgeRegion_->Draw();
    }

    // 機関銃の弾の描画
    if (bulletRegion_) {
        bulletRegion_->ClearInstances();
        for (int i = 0; i < kMaxBullets; ++i) {
            if (bullets_[i].isActive) {
                Vector3 bRot = { 0.0f, std::atan2(bullets_[i].velocity.x, bullets_[i].velocity.z), 0.0f };
                float bxzLen = std::sqrt(bullets_[i].velocity.x * bullets_[i].velocity.x + bullets_[i].velocity.z * bullets_[i].velocity.z);
                bRot.x = std::atan2(-bullets_[i].velocity.y, bxzLen);

                Transform tf;
                tf.translate = bullets_[i].position;
                tf.rotate = bRot;
                tf.scale = { 0.06f, 0.06f, 0.24f };
                bulletRegion_->AddInstance(tf, { 1.0f, 1.0f, 0.0f, 1.0f });
            }
        }
        bulletRegion_->Draw();
    }

    // ミサイルの描画
    if (missileRegion_) {
        missileRegion_->ClearInstances();
        for (int i = 0; i < kMaxMissiles; ++i) {
            if (missiles_[i].isActive) {
                Vector3 drawPos = missiles_[i].position;
                drawPos.y += missileVibration_.y; // スピード感を出すための振動

                Vector3 mRot = { 0.0f, std::atan2(missiles_[i].velocity.x, missiles_[i].velocity.z), 0.0f };
                float xzLen = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x + missiles_[i].velocity.z * missiles_[i].velocity.z);
                mRot.x = std::atan2(-missiles_[i].velocity.y, xzLen);

                Transform tf;
                tf.translate = drawPos;
                tf.rotate = mRot;
                tf.scale = { 0.15f, 0.15f, 0.8f };
                missileRegion_->AddInstance(tf, { 1.0f, 1.0f, 1.0f, 1.0f });
            }
        }
        missileRegion_->Draw();
    }
}

void PlayerWeapon::DrawParticles(IrufemiEngine* engine) {
    if (muzzleSmokeLeft_) muzzleSmokeLeft_->Draw();
    if (muzzleSmokeRight_) muzzleSmokeRight_->Draw();
    if (muzzleFlashLeft_) muzzleFlashLeft_->Draw();
    if (muzzleFlashRight_) muzzleFlashRight_->Draw();
    if (muzzleFlashAddLeft_) muzzleFlashAddLeft_->Draw();
    if (muzzleFlashAddRight_) muzzleFlashAddRight_->Draw();
    if (missileSmoke_) missileSmoke_->Draw();
    if (missileFire_) missileFire_->Draw();
    if (bulletTrail_) bulletTrail_->Draw();
    if (ejectionMistLeft_) ejectionMistLeft_->Draw();
    if (ejectionMistRight_) ejectionMistRight_->Draw();
}

void PlayerWeapon::UpdateMissile(const Vector3& targetPos, const Vector3& playerScale, bool isKarakuriCharged) {
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive) {
            // からくりチャージ中は射程（タイマー）を減らさない（または非常に長くする）
            if (isKarakuriCharged) {
                if (missiles_[i].timer < 60) missiles_[i].timer = 60;
            }

            // オートエイムのため、ターゲットの最新座標を更新
            missiles_[i].target = { targetPos.x, targetPos.y + 1.0f, targetPos.z };

            Vector3 toTarget = {
                missiles_[i].target.x - missiles_[i].position.x,
                missiles_[i].target.y - missiles_[i].position.y,
                missiles_[i].target.z - missiles_[i].position.z
            };

            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
            if (dist > 0.1f) {
                Vector3 desiredVelocity = {
                    (toTarget.x / dist) * kMissileSpeed,
                    (toTarget.y / dist) * kMissileSpeed,
                    (toTarget.z / dist) * kMissileSpeed
                };

                // 旋回性能（追尾）
                float turnSpeed = isKarakuriCharged ? missileTurnSpeedCharged_ : missileTurnSpeedNormal_; // 弧を描くように追尾速度を緩やかにする
                missiles_[i].velocity.x += (desiredVelocity.x - missiles_[i].velocity.x) * turnSpeed;
                missiles_[i].velocity.y += (desiredVelocity.y - missiles_[i].velocity.y) * turnSpeed;
                missiles_[i].velocity.z += (desiredVelocity.z - missiles_[i].velocity.z) * turnSpeed;
            }

            // 速度の正規化と適用
            float currentSpeed = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x +
                missiles_[i].velocity.y * missiles_[i].velocity.y +
                missiles_[i].velocity.z * missiles_[i].velocity.z);
            float targetSpeed = isKarakuriCharged ? kMissileSpeed * 1.5f : kMissileSpeed;
            if (currentSpeed > 0.0f) {
                missiles_[i].velocity.x = (missiles_[i].velocity.x / currentSpeed) * targetSpeed;
                missiles_[i].velocity.y = (missiles_[i].velocity.y / currentSpeed) * targetSpeed;
                missiles_[i].velocity.z = (missiles_[i].velocity.z / currentSpeed) * targetSpeed;
            }

            missiles_[i].position.x += missiles_[i].velocity.x;
            missiles_[i].position.y += missiles_[i].velocity.y;
            missiles_[i].position.z += missiles_[i].velocity.z;

            // 地面に潜らないようにする
            if (missiles_[i].position.y < 0.1f) {
                missiles_[i].position.y = 0.1f;
                if (missiles_[i].velocity.y < 0.0f) {
                    missiles_[i].velocity.y = 0.0f;
                }
            }

            float missileHalfLength = 2.2f; // 描画スケール(Z=0.8)に基づく固定値 (尻尾よりわずかに内側)
            float vx = missiles_[i].velocity.x;
            float vy = missiles_[i].velocity.y;
            float vz = missiles_[i].velocity.z;
            float speed = std::sqrt(vx * vx + vy * vy + vz * vz);
            Vector3 tailPos = missiles_[i].position;

            if (speed > 0.001f) {
                tailPos.x -= (vx / speed) * missileHalfLength;
                tailPos.y -= (vy / speed) * missileHalfLength;
                tailPos.z -= (vz / speed) * missileHalfLength;
            }

            if (missileFire_) missileFire_->PlayHitEffect(tailPos, 3);
            if (missileSmoke_) missileSmoke_->PlayHitEffect(tailPos, 2);

            missiles_[i].timer--;
            if (missiles_[i].timer <= 0) missiles_[i].isActive = false;
        }
    }
}

void PlayerWeapon::UpdateMachineGun(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos) {
    if (machineGunActiveTimer_ > 0) {
        // 残弾が尽きたら強制停止
        if (machineGunAmmo_ <= 0) {
            machineGunActiveTimer_ = 0;
        } else {
            machineGunActiveTimer_--;
            machineGunFireTimer_--;

            if (machineGunFireTimer_ <= 0) {
                machineGunFireTimer_ = 5; // 発射間隔

                machineGunVibration_.x = ((std::rand() % 100) / 100.0f - 0.5f) * machineGunVibrationScale_ * 2.0f;
                machineGunVibration_.y = ((std::rand() % 100) / 100.0f - 0.5f) * machineGunVibrationScale_ * 2.0f;
                machineGunVibration_.z = ((std::rand() % 100) / 100.0f - 0.5f) * machineGunVibrationScale_ * 2.0f;

                float sinY = std::sin(playerRotate.y);
                float cosY = std::cos(playerRotate.y);
                float rightX = cosY;
                float rightZ = -sinY;

                Vector3 leftShoulder = { playerTranslate.x - rightX * 0.7f, playerTranslate.y + 1.0f, playerTranslate.z - rightZ * 0.7f };
                Vector3 rightShoulder = { playerTranslate.x + rightX * 0.7f, playerTranslate.y + 1.0f, playerTranslate.z + rightZ * 0.7f };

                Vector3 playerCenter = { playerTranslate.x, playerTranslate.y + 1.0f, playerTranslate.z };
                Vector3 aimPos = { targetPos.x, targetPos.y + 1.0f, targetPos.z };
                Vector3 toTarget = { aimPos.x - playerCenter.x, aimPos.y - playerCenter.y, aimPos.z - playerCenter.z };

                Vector3 rot = { 0.0f, 0.0f, 0.0f };
                float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);

                float cosP = std::cos(cameraPitch);
                float sinP = std::sin(cameraPitch);
                Vector3 playerForward = { std::sin(playerRotate.y) * cosP, -sinP, std::cos(playerRotate.y) * cosP };

                Vector3 blendedForward = playerForward;

                if (dist > 0.001f) {
                    Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
                    float dot = playerForward.x * toTargetNorm.x + playerForward.y * toTargetNorm.y + playerForward.z * toTargetNorm.z;

                    float assistThreshold = 0.8f;
                    if (dot > assistThreshold) {
                        float assistRatio = (dot - assistThreshold) / (1.0f - assistThreshold);
                        assistRatio = std::pow(assistRatio, 1.5f);

                        blendedForward.x = playerForward.x * (1.0f - assistRatio) + toTargetNorm.x * assistRatio;
                        blendedForward.y = playerForward.y * (1.0f - assistRatio) + toTargetNorm.y * assistRatio;
                        blendedForward.z = playerForward.z * (1.0f - assistRatio) + toTargetNorm.z * assistRatio;

                        float fLen = std::sqrt(blendedForward.x * blendedForward.x + blendedForward.y * blendedForward.y + blendedForward.z * blendedForward.z);
                        blendedForward.x /= fLen;
                        blendedForward.y /= fLen;
                        blendedForward.z /= fLen;
                    }
                }

                rot.y = std::atan2(blendedForward.x, blendedForward.z);
                float xzLen = std::sqrt(blendedForward.x * blendedForward.x + blendedForward.z * blendedForward.z);
                rot.x = std::atan2(-blendedForward.y, xzLen);

                float muzzleOffsetSize = (kMachineGunModelSize.z * 0.5f) * kMachineGunScale.z;
                float cosRotX = std::cos(rot.x);
                Vector3 forward = { std::sin(rot.y) * cosRotX, -std::sin(rot.x), std::cos(rot.y) * cosRotX };

                Vector3 muzzleLeft = { leftShoulder.x + forward.x * muzzleOffsetSize, leftShoulder.y + forward.y * muzzleOffsetSize, leftShoulder.z + forward.z * muzzleOffsetSize };
                Vector3 muzzleRight = { rightShoulder.x + forward.x * muzzleOffsetSize, rightShoulder.y + forward.y * muzzleOffsetSize, rightShoulder.z + forward.z * muzzleOffsetSize };

                FireMachineGunBullet(muzzleLeft, playerTranslate, playerRotate, cameraPitch, targetPos);
                EjectCartridge(leftShoulder, false, playerTranslate, playerRotate, targetPos);

                if (muzzleFlashLeft_) muzzleFlashLeft_->PlayHitEffect(muzzleLeft);
                if (muzzleFlashAddLeft_) {
                    muzzleFlashAddLeft_->PlayHitEffect(muzzleLeft);
                    muzzleFlashAddLeft_->PlayHitEffect(muzzleLeft);
                }
                if (ejectionMistLeft_) ejectionMistLeft_->PlayHitEffect({ leftShoulder.x - rightX * 0.3f, leftShoulder.y, leftShoulder.z - rightZ * 0.3f });

                FireMachineGunBullet(muzzleRight, playerTranslate, playerRotate, cameraPitch, targetPos);
                EjectCartridge(rightShoulder, true, playerTranslate, playerRotate, targetPos);

                if (muzzleFlashRight_) muzzleFlashRight_->PlayHitEffect(muzzleRight);
                if (muzzleFlashAddRight_) {
                    muzzleFlashAddRight_->PlayHitEffect(muzzleRight);
                    muzzleFlashAddRight_->PlayHitEffect(muzzleRight);
                }
                if (ejectionMistRight_) ejectionMistRight_->PlayHitEffect({ rightShoulder.x + rightX * 0.3f, rightShoulder.y, rightShoulder.z + rightZ * 0.3f });

                // 発射ごとに残弾を消費（左右合わせて1発分）
                machineGunAmmo_--;
                if (machineGunAmmo_ < 0) machineGunAmmo_ = 0;
            }
        }
    } else {
        // 停止中: 残弾を自動回復
        if (machineGunAmmo_ < kMaxMachineGunAmmo) {
            machineGunAmmoRecoveryTimer_++;
            if (machineGunAmmoRecoveryTimer_ >= kAmmoRecoveryInterval) {
                machineGunAmmoRecoveryTimer_ = 0;
                machineGunAmmo_++;
                if (machineGunAmmo_ > kMaxMachineGunAmmo) machineGunAmmo_ = kMaxMachineGunAmmo;
            }
        }
    }

    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive) {
            bullets_[i].position.x += bullets_[i].velocity.x;
            bullets_[i].position.y += bullets_[i].velocity.y;
            bullets_[i].position.z += bullets_[i].velocity.z;

            if (bulletTrail_) bulletTrail_->PlayHitEffect(bullets_[i].position, 2);

            bullets_[i].timer--;
            if (bullets_[i].timer <= 0) {
                bullets_[i].isActive = false;
            }
        }
    }
}

void PlayerWeapon::FireMachineGunBullet(const Vector3& startPos, const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos) {
    for (int i = 0; i < kMaxBullets; ++i) {
        if (!bullets_[i].isActive) {
            bullets_[i].isActive = true;
            bullets_[i].timer = 60;
            bullets_[i].position = startPos;

            Vector3 aimPos = { targetPos.x, targetPos.y + 1.0f, targetPos.z };
            Vector3 playerCenter = { playerTranslate.x, playerTranslate.y + 1.0f, playerTranslate.z };
            Vector3 toTarget = { aimPos.x - playerCenter.x, aimPos.y - playerCenter.y, aimPos.z - playerCenter.z };

            float dist = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
            Vector3 forward;

            // ★修正: 照準に近いほど吸い寄せる（エイムアシスト）
            float sinY = std::sin(playerRotate.y);
            float cosY = std::cos(playerRotate.y);
            float cosP = std::cos(cameraPitch);
            float sinP = std::sin(cameraPitch);
            Vector3 playerForward = { sinY * cosP, -sinP, cosY * cosP };

            forward = playerForward; // デフォルトは照準（正面）方向

            if (dist > 0.001f) {
                Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
                float dot = playerForward.x * toTargetNorm.x + playerForward.y * toTargetNorm.y + playerForward.z * toTargetNorm.z;

                // 0.8f（正面から約36度以内）ならアシスト開始、1.0f（完全な真正面）で最大アシスト
                float assistThreshold = 0.8f;
                if (dot > assistThreshold) {
                    // 0.0(アシストなし) ～ 1.0(完全吸い付き) の割合を計算
                    float assistRatio = (dot - assistThreshold) / (1.0f - assistThreshold);

                    // 吸い付きをより自然にするため、カーブをかける（任意）
                    assistRatio = std::pow(assistRatio, 1.5f);

                    // 照準方向と敵方向のベクトルをブレンド（合成）
                    forward.x = playerForward.x * (1.0f - assistRatio) + toTargetNorm.x * assistRatio;
                    forward.y = playerForward.y * (1.0f - assistRatio) + toTargetNorm.y * assistRatio;
                    forward.z = playerForward.z * (1.0f - assistRatio) + toTargetNorm.z * assistRatio;

                    // 正規化
                    float fLen = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
                    forward.x /= fLen;
                    forward.y /= fLen;
                    forward.z /= fLen;
                }
            }

            float bulletSpeed = 5.0f;
            bullets_[i].velocity = { forward.x * bulletSpeed, forward.y * bulletSpeed, forward.z * bulletSpeed };

            // ばらつき
            float spread = 0.1f;
            bullets_[i].velocity.x += ((std::rand() % 100) / 100.0f - 0.5f) * spread;
            bullets_[i].velocity.y += ((std::rand() % 100) / 100.0f - 0.5f) * spread;
            bullets_[i].velocity.z += ((std::rand() % 100) / 100.0f - 0.5f) * spread;

            break;
        }
    }
}

void PlayerWeapon::UpdateCartridges() {
    for (int i = 0; i < kMaxCartridges; ++i) {
        if (cartridges_[i].isActive) {
            cartridges_[i].timer--;
            if (cartridges_[i].timer <= 0) {
                cartridges_[i].isActive = false;
                continue;
            }

            int fadeDuration = 15;
            bool isFading = (cartridges_[i].timer < fadeDuration);

            if (!isFading) {
                cartridges_[i].velocity.y -= kGravity;
            } else {
                cartridges_[i].velocity.y = -0.005f;
                cartridges_[i].velocity.x *= 0.9f;
                cartridges_[i].velocity.z *= 0.9f;
            }

            cartridges_[i].position.x += cartridges_[i].velocity.x;
            cartridges_[i].position.y += cartridges_[i].velocity.y;
            cartridges_[i].position.z += cartridges_[i].velocity.z;

            cartridges_[i].rotation.x += cartridges_[i].angularVelocity.x;
            cartridges_[i].rotation.y += cartridges_[i].angularVelocity.y;
            cartridges_[i].rotation.z += cartridges_[i].angularVelocity.z;

            if (cartridges_[i].position.y <= 0.0f) {
                if (!isFading) {
                    cartridges_[i].position.y = 0.0f;
                    cartridges_[i].velocity.y *= -0.4f;
                    cartridges_[i].velocity.x *= 0.7f;
                    cartridges_[i].velocity.z *= 0.7f;
                    cartridges_[i].angularVelocity.x *= 0.5f;
                    cartridges_[i].angularVelocity.y *= 0.5f;
                    cartridges_[i].angularVelocity.z *= 0.5f;
                }
            }
        }
    }
}

void PlayerWeapon::EjectCartridge(const Vector3& startPos, bool isRight, const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& targetPos) {
    for (int i = 0; i < kMaxCartridges; ++i) {
        if (!cartridges_[i].isActive) {
            cartridges_[i].isActive = true;
            cartridges_[i].timer = 60;
            cartridges_[i].position = startPos;

            float sinY = std::sin(playerRotate.y);
            float cosY = std::cos(playerRotate.y);

            Vector3 rightDir = { cosY, 0.0f, -sinY };
            Vector3 ejectDir = isRight ? rightDir : Vector3{ -rightDir.x, -rightDir.y, -rightDir.z };

            float ejectSpeed = 0.2f + ((std::rand() % 100) / 1000.0f);
            cartridges_[i].velocity = {
                ejectDir.x * ejectSpeed + ((std::rand() % 100) / 1000.0f - 0.05f),
                0.3f + ((std::rand() % 100) / 1000.0f),
                ejectDir.z * ejectSpeed + ((std::rand() % 100) / 1000.0f - 0.05f)
            };

            cartridges_[i].rotation = {
                (std::rand() % 360) * 3.14159f / 180.0f,
                (std::rand() % 360) * 3.14159f / 180.0f,
                (std::rand() % 360) * 3.14159f / 180.0f
            };

            cartridges_[i].angularVelocity = {
                ((std::rand() % 100) / 50.0f - 1.0f) * 0.5f,
                ((std::rand() % 100) / 50.0f - 1.0f) * 0.5f,
                ((std::rand() % 100) / 50.0f - 1.0f) * 0.5f
            };

            break;
        }
    }
}

void PlayerWeapon::StartMachineGunSkill() {
    machineGunActiveTimer_ = 180;
    machineGunVibrationScale_ = 0.1f;
}

void PlayerWeapon::StopMachineGunSkill() {
    machineGunActiveTimer_ = 0;
}

void PlayerWeapon::FireMissileSkill(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& targetPos) {
    missileVibrationTimer_ = kMissileVibrationDuration;

    float sinY = std::sin(playerRotate.y);
    float cosY = std::cos(playerRotate.y);

    int firedCount = 0; // 今回発射した数をカウント

    for (int i = 0; i < kMaxMissiles; ++i) {
        if (!missiles_[i].isActive) {
            missiles_[i].isActive = true;
            missiles_[i].timer = 120;
            missiles_[i].target = { targetPos.x, targetPos.y + 1.0f, targetPos.z };

            // 発射位置をプレイヤーの周囲に分散させる
            float offsetR = ((std::rand() % 100) / 100.0f) * 3.0f + 2.0f; // 半径 2.0 ～ 5.0 (より外側に)
            float offsetTheta = ((std::rand() % 360) * 3.14159f / 180.0f);
            float offsetX = std::cos(offsetTheta) * offsetR;
            float offsetY = ((std::rand() % 100) / 100.0f) * 3.0f; // 高さ 0.0 ～ 3.0
            float offsetZ = std::sin(offsetTheta) * offsetR;

            missiles_[i].position = { playerTranslate.x + offsetX, playerTranslate.y + offsetY, playerTranslate.z + offsetZ };

            // 初速度を大きく広がる方向に設定
            float spreadMagnitude = missileSpreadMagnitudeBase_ + ((std::rand() % 100) / 100.0f) * missileSpreadMagnitudeRand_; // 外側に飛ぶ力を強くする
            Vector3 spreadDir = { offsetX, offsetY + 1.5f, offsetZ }; // 外側かつ上向きに膨らませる
            float len = std::sqrt(spreadDir.x * spreadDir.x + spreadDir.y * spreadDir.y + spreadDir.z * spreadDir.z);
            if (len > 0.001f) {
                spreadDir.x /= len; spreadDir.y /= len; spreadDir.z /= len;
            }

            missiles_[i].velocity = { spreadDir.x * spreadMagnitude, spreadDir.y * spreadMagnitude, spreadDir.z * spreadMagnitude };

            firedCount++;
            // 1回のスキル呼び出しにつき4発発射（合計で8または16発になるように調整）
            if (firedCount >= 4) {
                break;
            }
        }
    }
}