#include "PlayerWeapon.h"
#include "Engine/Core/Math/Geometry/Math.h"
#include "Renderer/Particle/ParticleSystem.h"
#include <cmath>
#include <cstdlib>

void PlayerWeapon::Initialize(Camera* camera) {
    camera_ = camera;

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

    // --- 銃口の煙パーティクルの初期化 ---
    muzzleSmokeLeft_ = std::make_unique<ParticleSystem>();
    muzzleSmokeLeft_->Initialize(camera_, "resources/circle.png", ParticleType::kMuzzleSmoke);
    muzzleSmokeRight_ = std::make_unique<ParticleSystem>();
    muzzleSmokeRight_->Initialize(camera_, "resources/circle.png", ParticleType::kMuzzleSmoke);

    // --- マズルフラッシュパーティクルの初期化 ---
    muzzleFlashLeft_ = std::make_unique<ParticleSystem>();
    muzzleFlashLeft_->Initialize(camera_, "resources/whiteTexture.png", ParticleType::kMuzzleFlash, PrimitiveType::Circle);
    muzzleFlashLeft_->SetBlend(BlendMode::kBlendModeNormal);
    muzzleFlashRight_ = std::make_unique<ParticleSystem>();
    muzzleFlashRight_->Initialize(camera_, "resources/whiteTexture.png", ParticleType::kMuzzleFlash, PrimitiveType::Circle);
    muzzleFlashRight_->SetBlend(BlendMode::kBlendModeNormal);

    // --- 加算合成マズルフラッシュの初期化 ---
    muzzleFlashAddLeft_ = std::make_unique<ParticleSystem>();
    muzzleFlashAddLeft_->Initialize(camera_, "resources/circle.png", ParticleType::kMuzzleFlash);
    muzzleFlashAddLeft_->SetBlend(BlendMode::kBlendModeAdd);
    muzzleFlashAddRight_ = std::make_unique<ParticleSystem>();
    muzzleFlashAddRight_->Initialize(camera_, "resources/circle.png", ParticleType::kMuzzleFlash);
    muzzleFlashAddRight_->SetBlend(BlendMode::kBlendModeAdd);

    missileFire_ = std::make_unique<ParticleSystem>();
    missileFire_->Initialize(camera_, "resources/circle.png", ParticleType::kMissileFire);

    missileSmoke_ = std::make_unique<ParticleSystem>();
    missileSmoke_->Initialize(camera_, "resources/circle.png", ParticleType::kMissileSmoke);

    bulletTrail_ = std::make_unique<ParticleSystem>();
    bulletTrail_->Initialize(camera_, "resources/circle.png", ParticleType::kBulletTrail);
    bulletTrail_->SetBlend(BlendMode::kBlendModeAdd);

    ejectionMistLeft_ = std::make_unique<ParticleSystem>();
    ejectionMistLeft_->Initialize(camera_, "resources/circle.png", ParticleType::kEjectionMist);
    ejectionMistLeft_->SetBlend(BlendMode::kBlendModeAdd);

    ejectionMistRight_ = std::make_unique<ParticleSystem>();
    ejectionMistRight_->Initialize(camera_, "resources/circle.png", ParticleType::kEjectionMist);
    ejectionMistRight_->SetBlend(BlendMode::kBlendModeAdd);

    // --- 薬莢モデルの初期化 ---
    for (int i = 0; i < kMaxCartridges; ++i) {
        cartridgeObjs_[i] = std::make_unique<ObjClass>();
        cartridgeObjs_[i]->Initialize(camera_, "enemy/body.obj");
        cartridgeObjs_[i]->SetColor({ 0.8f, 0.6f, 0.1f, 1.0f });
        cartridges_[i].isActive = false;
    }

    // --- ミサイルモデルとデータの初期化 ---
    for (int i = 0; i < kMaxMissiles; ++i) {
        missileObjs_[i] = std::make_unique<ObjClass>();
        missileObjs_[i]->Initialize(camera_, "enemy/body.obj");
        missiles_[i].isActive = false;
    }

    machineGunVibration_ = { 0.0f, 0.0f, 0.0f };
    missileVibration_ = { 0.0f, 0.0f, 0.0f };
    missileVibrationTimer_ = 0;
}

void PlayerWeapon::Update(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos, const Vector3& playerScale) {
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
    UpdateMissile(targetPos, playerScale);
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

        // ★修正: 敵が画面内（正面から約60度以内）にいるか判定
        bool isEnemyOnScreen = false;
        if (dist > 0.001f) {
            float cosP = std::cos(cameraPitch);
            Vector3 forwardP = { std::sin(playerRotate.y) * cosP, -std::sin(cameraPitch), std::cos(playerRotate.y) * cosP };
            Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };

            // 内積(Dot)で角度をチェック。0.5f は視野角120度相当
            float dot = forwardP.x * toTargetNorm.x + forwardP.y * toTargetNorm.y + forwardP.z * toTargetNorm.z;
            if (dot > 0.5f) {
                isEnemyOnScreen = true;
            }
        }

        // 画面内にいるなら敵を向き、そうでないならカメラ（プレイヤー）の正面を向く
        if (isEnemyOnScreen) {
            rot.y = std::atan2(toTarget.x, toTarget.z);
            float xzLen = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
            rot.x = std::atan2(-toTarget.y, xzLen);
        } else {
            rot.y = playerRotate.y;
            rot.x = cameraPitch;
        }

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
    for (int i = 0; i < kMaxCartridges; ++i) {
        if (cartridges_[i].isActive && cartridgeObjs_[i]) {
            cartridgeObjs_[i]->SetPosition(cartridges_[i].position);
            cartridgeObjs_[i]->SetRotate(cartridges_[i].rotation);
            cartridgeObjs_[i]->SetScale({ 0.02f, 0.02f, 0.05f });
            cartridgeObjs_[i]->Update();
            cartridgeObjs_[i]->Draw();
        }
    }

    // 機関銃の弾の描画
    for (int i = 0; i < kMaxBullets; ++i) {
        if (bullets_[i].isActive && bulletObjs_[i]) {
            bulletObjs_[i]->SetPosition(bullets_[i].position);
            Vector3 bRot = { 0.0f, std::atan2(bullets_[i].velocity.x, bullets_[i].velocity.z), 0.0f };
            float bxzLen = std::sqrt(bullets_[i].velocity.x * bullets_[i].velocity.x + bullets_[i].velocity.z * bullets_[i].velocity.z);
            bRot.x = std::atan2(-bullets_[i].velocity.y, bxzLen);
            bulletObjs_[i]->SetRotate(bRot);
            bulletObjs_[i]->SetScale({ 0.06f, 0.06f, 0.24f }); // 弾本体をさらに小型化
            bulletObjs_[i]->Update();
            bulletObjs_[i]->Draw();
        }
    }

    // ミサイルの描画
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive && missileObjs_[i]) {
            missiles_[i].position.y += missileVibration_.y; // スピード感を出すための振動
            missileObjs_[i]->SetPosition(missiles_[i].position);
            Vector3 mRot = { 0.0f, std::atan2(missiles_[i].velocity.x, missiles_[i].velocity.z), 0.0f };
            float xzLen = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x + missiles_[i].velocity.z * missiles_[i].velocity.z);
            mRot.x = std::atan2(-missiles_[i].velocity.y, xzLen);
            missileObjs_[i]->SetRotate(mRot);
            Vector3 missileScale = { 0.15f, 0.15f, 0.8f };
            missileObjs_[i]->SetScale(missileScale);
            missileObjs_[i]->Update();
            missileObjs_[i]->Draw();
        }
    }
}

void PlayerWeapon::DrawParticles(IrufemiEngine* engine) {
    if (muzzleSmokeLeft_) muzzleSmokeLeft_->Draw();
    if (muzzleSmokeRight_) muzzleSmokeRight_->Draw();
    if (muzzleFlashLeft_) muzzleFlashLeft_->Draw();
    if (muzzleFlashRight_) muzzleFlashRight_->Draw();
    if (muzzleFlashAddLeft_) muzzleFlashAddLeft_->Draw();
    if (muzzleFlashAddRight_) muzzleFlashAddRight_->Draw();
    if (missileFire_) missileFire_->Draw();
    if (missileSmoke_) missileSmoke_->Draw();
    if (bulletTrail_) bulletTrail_->Draw();
    if (ejectionMistLeft_) ejectionMistLeft_->Draw();
    if (ejectionMistRight_) ejectionMistRight_->Draw();
}

void PlayerWeapon::UpdateMissile(const Vector3& targetPos, const Vector3& playerScale) {
    for (int i = 0; i < kMaxMissiles; ++i) {
        if (missiles_[i].isActive) {
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

                // 旋回性能
                float turnSpeed = 0.05f;
                missiles_[i].velocity.x += (desiredVelocity.x - missiles_[i].velocity.x) * turnSpeed;
                missiles_[i].velocity.y += (desiredVelocity.y - missiles_[i].velocity.y) * turnSpeed;
                missiles_[i].velocity.z += (desiredVelocity.z - missiles_[i].velocity.z) * turnSpeed;
            }

            // 速度の正規化
            float currentSpeed = std::sqrt(missiles_[i].velocity.x * missiles_[i].velocity.x +
                missiles_[i].velocity.y * missiles_[i].velocity.y +
                missiles_[i].velocity.z * missiles_[i].velocity.z);
            if (currentSpeed > 0.0f) {
                missiles_[i].velocity.x = (missiles_[i].velocity.x / currentSpeed) * kMissileSpeed;
                missiles_[i].velocity.y = (missiles_[i].velocity.y / currentSpeed) * kMissileSpeed;
                missiles_[i].velocity.z = (missiles_[i].velocity.z / currentSpeed) * kMissileSpeed;
            }

            missiles_[i].position.x += missiles_[i].velocity.x;
            missiles_[i].position.y += missiles_[i].velocity.y;
            missiles_[i].position.z += missiles_[i].velocity.z;

            float missileHalfLength = (6.0f * 0.5f) * (playerScale.z * 0.4f);
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

            // ★修正: 弾を発射する位置を決めるためにも画面内判定を使う
            bool isEnemyOnScreen = false;
            if (dist > 0.001f) {
                float cosP = std::cos(cameraPitch);
                Vector3 forwardP = { std::sin(playerRotate.y) * cosP, -std::sin(cameraPitch), std::cos(playerRotate.y) * cosP };
                Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
                float dot = forwardP.x * toTargetNorm.x + forwardP.y * toTargetNorm.y + forwardP.z * toTargetNorm.z;
                if (dot > 0.5f) {
                    isEnemyOnScreen = true;
                }
            }

            if (isEnemyOnScreen) {
                rot.y = std::atan2(toTarget.x, toTarget.z);
                float xzLen = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
                rot.x = std::atan2(-toTarget.y, xzLen);
            } else {
                rot.y = playerRotate.y;
                rot.x = cameraPitch;
            }

            float muzzleOffsetSize = (kMachineGunModelSize.z * 0.5f) * kMachineGunScale.z;
            float cosRotX = std::cos(rot.x);
            Vector3 forward = { std::sin(rot.y) * cosRotX, -std::sin(rot.x), std::cos(rot.y) * cosRotX };

            Vector3 muzzleLeft = { leftShoulder.x + forward.x * muzzleOffsetSize, leftShoulder.y + forward.y * muzzleOffsetSize, leftShoulder.z + forward.z * muzzleOffsetSize };
            Vector3 muzzleRight = { rightShoulder.x + forward.x * muzzleOffsetSize, rightShoulder.y + forward.y * muzzleOffsetSize, rightShoulder.z + forward.z * muzzleOffsetSize };

            FireMachineGunBullet(muzzleLeft, playerTranslate, playerRotate, cameraPitch, targetPos);
            EjectCartridge(leftShoulder, false, playerTranslate, playerRotate, targetPos);

            // if (muzzleSmokeLeft_) muzzleSmokeLeft_->PlayHitEffect(leftShoulder); // 既存の煙を停止
            if (muzzleFlashLeft_) muzzleFlashLeft_->PlayHitEffect(muzzleLeft);
            if (muzzleFlashAddLeft_) {
                muzzleFlashAddLeft_->PlayHitEffect(muzzleLeft);
                muzzleFlashAddLeft_->PlayHitEffect(muzzleLeft);
            }
            if (ejectionMistLeft_) ejectionMistLeft_->PlayHitEffect({ leftShoulder.x - rightX * 0.3f, leftShoulder.y, leftShoulder.z - rightZ * 0.3f });

            FireMachineGunBullet(muzzleRight, playerTranslate, playerRotate, cameraPitch, targetPos);
            EjectCartridge(rightShoulder, true, playerTranslate, playerRotate, targetPos);

            // if (muzzleSmokeRight_) muzzleSmokeRight_->PlayHitEffect(rightShoulder); // 既存の煙を停止
            if (muzzleFlashRight_) muzzleFlashRight_->PlayHitEffect(muzzleRight);
            if (muzzleFlashAddRight_) {
                muzzleFlashAddRight_->PlayHitEffect(muzzleRight);
                muzzleFlashAddRight_->PlayHitEffect(muzzleRight);
            }
            if (ejectionMistRight_) ejectionMistRight_->PlayHitEffect({ rightShoulder.x + rightX * 0.3f, rightShoulder.y, rightShoulder.z + rightZ * 0.3f });
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

            // ★修正: 弾の軌道も敵が画面内にいるか判断して撃ち分ける
            bool isEnemyOnScreen = false;
            float sinY = std::sin(playerRotate.y);
            float cosY = std::cos(playerRotate.y);
            float cosP = std::cos(cameraPitch);
            float sinP = std::sin(cameraPitch);
            Vector3 playerForward = { sinY * cosP, -sinP, cosY * cosP };

            if (dist > 0.001f) {
                Vector3 toTargetNorm = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
                float dot = playerForward.x * toTargetNorm.x + playerForward.y * toTargetNorm.y + playerForward.z * toTargetNorm.z;
                if (dot > 0.5f) {
                    isEnemyOnScreen = true;
                }
            }

            if (isEnemyOnScreen) {
                forward = { toTarget.x / dist, toTarget.y / dist, toTarget.z / dist };
            } else {
                forward = playerForward;
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
            missiles_[i].position = { playerTranslate.x + sinY * 1.0f, playerTranslate.y + 1.0f, playerTranslate.z + cosY * 1.0f };

            float spreadX = ((std::rand() % 100) / 25.0f) - 2.0f;
            float spreadY = ((std::rand() % 100) / 25.0f) - 0.5f;
            float spreadZ = ((std::rand() % 100) / 25.0f) - 2.0f;

            missiles_[i].velocity = { (sinY * 0.2f) + (spreadX * 0.4f), spreadY * 0.4f, (cosY * 0.2f) + (spreadZ * 0.4f) };

            firedCount++;
            // 1回のスキル呼び出しにつき4発発射したら終了する
            if (firedCount >= 4) {
                break;
            }
        }
    }
}