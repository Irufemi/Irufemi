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

/**
 * @struct AttackCollision
 * @brief プレイヤーの攻撃判定（攻撃を当てる側）データ
 */
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

    // ゲッター
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
    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }
    void HitAndKnockback(Enemy* enemy);

private:
    void HandleMovement();
    void HandleAttack();
    void HandleSkill();

private:
    InputManager* input_ = nullptr;
    IrufemiEngine* engine_ = nullptr;

    // --- コンポーネント群 ---
    PlayerMovement movement_;
    PlayerWeapon weapon_;
    PlayerCamera cameraController_;
    PlayerStatus status_;

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

    // --- 攻撃用定数（マジックナンバー解消） ---
    static constexpr float kMaxChargeTime = 60.0f;               // チャージ最大フレーム数
    static constexpr float kHammerAngleOffset = 1.5f;            // 構え時の角度オフセット
    static constexpr float kSwingTotalAngle = 3.0f;              // スイング時の総回転角度
    static constexpr float kHammerRotX = 1.57f;                  // ハンマーのX軸傾き

    static constexpr float kSwingBaseRadius = 2.5f;              // 基本スイング半径
    static constexpr float kSwingRadiusChargeBonus = 0.5f;       // チャージによる半径増加量

    static constexpr float kHammerBaseHeight = 1.0f;             // ハンマーの基準高さ
    static constexpr float kHammerSwaySpeed = 0.5f;              // 構え中の揺れ速度
    static constexpr float kHammerSwayAmplitude = 0.1f;          // 構え中の揺れ幅

    static constexpr float kHammerBaseSize = 0.8f;               // ハンマーの基本サイズ
    static constexpr float kHammerSizeChargeBonus = 0.4f;        // チャージによるサイズ増加量
    static constexpr float kHammerScaleYMultiplier = 1.5f;       // ハンマーの縦長スケール倍率

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    bool isDebugDrawOBB_ = false;
#endif
};