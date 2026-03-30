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

struct AttackCollision {
    Vector3 center;
    float radius;
    bool isActive;
};

class Player {
public:
    ~Player();

    void Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine);
    void Update();
    void Draw();
    void DrawParticles();

    const Vector3& GetTranslate() const { return translate_; }
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

    void ApplyDamage(int damage);

    // ターゲット設定関係
    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }
    void SetIsTargetingEnemy(bool isTargeting) { isTargetingEnemy_ = isTargeting; }
    bool GetIsTargetingEnemy() const { return isTargetingEnemy_; }

    void HitAndKnockback(Enemy* enemy);

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

    std::unique_ptr<ObjClass> obj_ = nullptr;
    std::unique_ptr<ObjClass> attackObj_ = nullptr;
    std::unique_ptr<ObjClass> targetMarkerObj_ = nullptr;
    std::unique_ptr<Sprite> maskSprite_ = nullptr;

    // ★ポイント: 照準座標を「ミサイル用」と「機関銃用」で分ける
    Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f }; // ミサイルのオートエイム用（常に敵の座標が入る）
    Vector3 aimPos_ = { 0.0f, 0.0f, 0.0f };    // 機関銃とマーカー用（画面外なら前方になる）

    bool isTargetingEnemy_ = false;

    int skillDurationTimer_ = 0;
    int skillCooldownTimer_ = 0;
    const int kSkillCooldownTime = 300;

    int karakuriChargeTimer_ = 0;
    const int kKarakuriChargeTime = 300;
    bool isKarakuriCharged_ = false;

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

    static constexpr float kSwingBaseRadius = 2.5f;
    static constexpr float kSwingRadiusChargeBonus = 0.5f;

    static constexpr float kHammerBaseHeight = 1.0f;
    static constexpr float kHammerSwaySpeed = 0.5f;
    static constexpr float kHammerSwayAmplitude = 0.1f;

    static constexpr float kHammerBaseSize = 0.8f;
    static constexpr float kHammerSizeChargeBonus = 0.4f;
    static constexpr float kHammerScaleYMultiplier = 1.5f;

    static constexpr float kModelOffsetY = 0.4f;              // モデル描画位置のY軸オフセット
    static constexpr float kAimDistance = 100.0f;             // 画面外の場合の照準距離
    static constexpr int kMissileSkillDuration = 120;         // ミサイルスキルの持続時間
    static constexpr int kMachineGunSkillDuration = 180;      // 機関銃スキルの持続時間

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    bool isDebugDrawOBB_ = false;
#endif
};