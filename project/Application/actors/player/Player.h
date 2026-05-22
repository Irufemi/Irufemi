#pragma once

#include "Irufemi.h"
#include "PlayerMovement.h" 
#include "PlayerWeapon.h" 
#include "PlayerCamera.h" 
#include "PlayerStatus.h" 
#include <memory>
#include <vector>

// 前方宣言
class Camera;
class Line3DRegion;
class Enemy;
class Sprite;
class PlayerHPBar;
class WeaponTrail;
class GPUParticleSystem;
class Effect;

struct AttackCollision {
    Vector3 center;
    float radius;
    bool isActive;
};

class Player {
public:
    Player();
    ~Player();

    void Initialize(InputManager* input, IrufemiEngine* engine);
    void Update();
    void Draw();
    void DrawParticles();
    void Draw3DUI(Enemy* enemy = nullptr, bool isUI = false, bool isPaused = false);
    void Draw2DUI(Enemy* enemy = nullptr);

    // ★追加: 弾丸・ミサイル着弾時の3D爆発エフェクト再生
    void PlayExplosion(const Vector3& position, float scale = 1.0f);

    const Vector3& GetTranslate() const { return translate_; }
    void SetTranslate(const Vector3& pos) { translate_ = pos; }
    const Vector3& GetRotate() const { return rotate_; }

    const AttackCollision& GetAttackCollision() const { return attackCollision_; }

    MachineGunBullet* GetMachineGunBullets() { return weapon_.GetMachineGunBullets(); }
    static int GetMaxMachineGunBullets() { return PlayerWeapon::GetMaxMachineGunBullets(); }
    MissileData* GetMissiles() { return weapon_.GetMissiles(); }
    static int GetMaxMissiles() { return PlayerWeapon::GetMaxMissiles(); }


    PlayerCollider GetCollider() const { return status_.GetCollider(translate_, rotate_, weapon_.GetMissileVibration()); }
    int GetHp() const { return status_.GetHp(); }
    int GetMaxHp() const { return status_.GetMaxHp(); }
    bool IsDead() const { return status_.IsDead(); }

    bool IsDeathAnimationFinished() const { return isDeathAnimationFinished_; }

    bool ApplyDamage(int damage);

    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }
    void SetIsTargetingEnemy(bool isTargeting) { isTargetingEnemy_ = isTargeting; }
    bool GetIsTargetingEnemy() const { return isTargetingEnemy_; }
    bool IsFirstPerson() const { return cameraController_.IsFirstPerson(); }

    bool IsKarakuriCharged() const { return isKarakuriCharged_; }
    int GetKarakuriActiveTimer() const { return karakuriActiveTimer_; }

    int GetCooldownWarningTimer() const { return cooldownWarningTimer_; }

    // --- チュートリアル用メソッド ---
    void ResetSkillCooldown() { skillCooldownTimer_ = 0; cooldownWarningTimer_ = 0; }
    void ForceKarakuriCharge() {
        isKarakuriCharged_ = true;
        karakuriActiveTimer_ = kKarakuriActiveTime;
    }
    void ResetDodgeCooldown() { movement_.ResetDodgeCooldown(); }

    void HitAndKnockback(Enemy* enemy);

    int GetDamageMelee() const { return damageMelee_; }
    float GetDamageMeleeChargeMultiplier() const { return damageMeleeChargeMultiplier_; }
    int GetDamageMachineGun() const { return damageMachineGun_; }
    float GetDamageMachineGunChargeMultiplier() const { return damageMachineGunChargeMultiplier_; }
    int GetDamageMissile() const { return damageMissile_; }
    float GetDamageMissileChargeMultiplier() const { return damageMissileChargeMultiplier_; }

private:
    void HandleMovement();
    void HandleAttack();
    void HandleSkill();

private:
    InputManager* input_ = nullptr;

    IrufemiEngine* engine_ = nullptr;

    PlayerMovement movement_;
    PlayerWeapon weapon_;
    PlayerCamera cameraController_;
    PlayerStatus status_;

    // ★追加: 3D爆発エフェクトプール
    std::vector<std::unique_ptr<Effect>> explosionEffects_;
    static const int kMaxExplosionEffects = 32;

    std::unique_ptr<ObjClass> obj_ = nullptr;
    std::unique_ptr<ObjClass> attackObj_ = nullptr;
    std::unique_ptr<ObjClass> targetMarkerObj_ = nullptr;
    std::unique_ptr<Sprite> maskSprite_ = nullptr;
    std::unique_ptr<PlayerHPBar> hpBar_ = nullptr;
    std::unique_ptr<WeaponTrail> weaponTrail_ = nullptr;

    Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 aimPos_ = { 0.0f, 0.0f, 0.0f };

    std::unique_ptr<Sprite> aimingSprite_ = nullptr;
    bool isTargetingEnemy_ = false;

    int skillDurationTimer_ = 0;
    int skillCooldownTimer_ = 0;
    const int kSkillCooldownTime = 300;
    int cooldownWarningTimer_ = 0;

    int karakuriChargeTimer_ = 0;
    const int kKarakuriChargeTime = 300;
    bool isKarakuriCharged_ = false;

    // ★追加: からくりチャージ用のゲージUI
    std::unique_ptr<Sprite> karakuriGaugeBg_ = nullptr;
    std::unique_ptr<Sprite> karakuriGaugeFill_ = nullptr;

    int karakuriActiveTimer_ = 0;
    const int kKarakuriActiveTime = 1200;

    Vector3 scale_ = { 0.3f, 0.5f, 0.3f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, -50.0f };

    enum class AttackState {
        kNone,
        kCharging,
        kAttacking
    };
    AttackState attackState_ = AttackState::kNone;
    int chargeTimer_ = 0;
    float currentChargeRate_ = 0.0f;

    AttackCollision attackCollision_ = {};
    int attackActiveTimer_ = 0;
    const int kAttackDuration = 20;

    static constexpr float kMaxChargeTime = 60.0f;
    static constexpr float kHammerAngleOffset = 1.5f;
    static constexpr float kSwingTotalAngle = 3.0f;
    static constexpr float kHammerRotX = 1.57f;

    static constexpr float kSwingBaseRadius = 4.0f;
    static constexpr float kSwingRadiusChargeBonus = 1.5f;

    static constexpr float kHammerBaseHeight = 1.0f;
    static constexpr float kHammerSwaySpeed = 0.5f;
    static constexpr float kHammerSwayAmplitude = 0.1f;

    float hammerBaseSize_ = 1.6f;
    float hammerSizeChargeBonus_ = 0.8f;
    float hammerScaleYMultiplier_ = 2.5f;

    // --- ダメージ関係の変数化 ---
    int damageMelee_ = 20;
    float damageMeleeChargeMultiplier_ = 2.5f;
    int damageMachineGun_ = 3;
    float damageMachineGunChargeMultiplier_ = 1.5f;
    int damageMissile_ = 50;
    float damageMissileChargeMultiplier_ = 2.0f;

    static constexpr float kModelOffsetY = 0.4f;
    static constexpr float kAimDistance = 100.0f;
    static constexpr int kMissileSkillDuration = 120;
    static constexpr int kMachineGunSkillDuration = 180;
    static constexpr int kMinAmmoToRestart = 5; // 再発射に必要な最低残弾数

    bool isMachineGunSkillActive_ = false; // 現在のスキルが機関銃かミサイルか

    // --- 死亡時の演出用変数 ---
    int deathTimer_ = 0;
    Vector3 deathVelocity_ = { 0.0f, 0.0f, 0.0f };
    Vector3 deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f };
    float deathYaw_ = 0.0f;
    bool isDeathAnimationFinished_ = false;
    static constexpr int kDeathAnimationDuration = 180;
    static constexpr int kDeathWaitTime = 90; // 死亡演出開始まで1.5秒待機（60fps x 1.5）
    int deathWaitTimer_ = 0; // 死亡待機タイマー

    Vector3 deathCameraPos_ = { 0.0f, 0.0f, 0.0f };

    // ★追加: キラン☆演出用の星モデル (plane.obj)
    std::unique_ptr<ObjClass> starObj_ = nullptr;
    Vector3 starScale_ = { 0.0f, 0.0f, 0.0f };
    float starRotationZ_ = 0.0f;

    // ★追加: からくりチャージ用のエフェクト
    std::unique_ptr<GPUParticleSystem> karakuriChargeParticle_ = nullptr; // チャージ中・完了時の上昇パーティクル
    std::unique_ptr<GPUParticleSystem> karakuriRingParticle_ = nullptr; // チャージしきったときの足元リングエフェクト
    std::unique_ptr<GPUParticleSystem> deathGlowParticle_ = nullptr; // 死亡待機中の全身から吹き出す自爆前光線

    // 一人称視点時のミニフィギュア設定
    Vector3 firstPersonMiniPos_ = { -0.58f, -0.21f, 1.4f };
    Vector3 firstPersonMiniScale_ = { 0.05f, 0.05f, 0.05f };
    float firstPersonMiniRotY_ = 0.5f;

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    bool isDebugDrawOBB_ = false;
#endif
};