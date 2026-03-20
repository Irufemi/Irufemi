#pragma once

#include "Irufemi.h"
#include "Engine/Core/Math/Geometry/OBB.h" 
#include "PlayerMovement.h" 
#include "PlayerWeapon.h" 
#include "PlayerCamera.h" // ★追加
#include <memory>
#include <vector>

// 前方宣言
class Camera;
class Line3DRegion;
class Enemy;

/**
 * @struct AttackCollision
 * @brief プレイヤーの攻撃判定（攻撃を当てる側）データ
 */
struct AttackCollision {
    Vector3 center;
    float radius;
    bool isActive;
};

/**
 * @struct PlayerCollider
 * @brief プレイヤー自身の当たり判定（攻撃を受ける側）データ
 */
struct PlayerCollider {
    Vector3 center;
    float radius;
    OBB obb;
};

class Player {
public:
    ~Player();

    void Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine);
    void Update();
    void Draw();
    void DrawParticles();

    // ゲッター
    const Vector3& GetTranslate() const { return translate_; }
    const Vector3& GetRotate() const { return rotate_; }

    const AttackCollision& GetAttackCollision() const { return attackCollision_; }

    MachineGunBullet* GetMachineGunBullets() { return weapon_.GetMachineGunBullets(); }
    static int GetMaxMachineGunBullets() { return PlayerWeapon::GetMaxMachineGunBullets(); }
    MissileData* GetMissiles() { return weapon_.GetMissiles(); }
    static int GetMaxMissiles() { return PlayerWeapon::GetMaxMissiles(); }

    PlayerCollider GetCollider() const;
    void ApplyDamage(int damage);

    int GetHp() const { return hp_; }
    int GetMaxHp() const { return kMaxHp; }
    bool IsDead() const { return isDead_; }

    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }
    void HitAndKnockback(Enemy* enemy);

private:
    void HandleMovement();
    void HandleAttack();
    void HandleSkill();

private:
    InputManager* input_ = nullptr;
    Camera* camera_ = nullptr;
    IrufemiEngine* engine_ = nullptr;

    // --- コンポーネント群 ---
    PlayerMovement movement_;
    PlayerWeapon weapon_;
    PlayerCamera cameraController_; // ★追加: カメラを管理するコンポーネント

    // 3Dモデル本体と分身
    std::unique_ptr<ObjClass> obj_ = nullptr;
    std::unique_ptr<ObjClass> attackObj_ = nullptr;

    // --- 一人称視点用マスク画像スプライト ---
    std::unique_ptr<Sprite> maskSprite_ = nullptr;

    Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };

    // --- スキルとからくりチャージ用 ---
    int skillDurationTimer_ = 0;
    int skillCooldownTimer_ = 0;
    const int kSkillCooldownTime = 300;

    int karakuriChargeTimer_ = 0;
    const int kKarakuriChargeTime = 300;
    bool isKarakuriCharged_ = false;

    int karakuriActiveTimer_ = 0;
    const int kKarakuriActiveTime = 1200;

    // トランスフォーム
    Vector3 scale_ = { 0.3f, 1.0f, 0.3f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, -50.0f };

    // --- 近接攻撃判定用 ---
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

    // --- ステータス・やられ判定用 ---
    const int kMaxHp = 100;
    int hp_ = kMaxHp;
    bool isDead_ = false;
    int invincibleTimer_ = 0;
    const float kColliderRadius = 1.0f;

    // --- ノックバック（吹き飛ばし）処理用変数 ---
    Enemy* knockbackTarget_ = nullptr;
    Vector3 knockbackVelocity_ = { 0.0f, 0.0f, 0.0f };
    int knockbackTimer_ = 0;

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    bool isDebugDrawOBB_ = false;
#endif
};