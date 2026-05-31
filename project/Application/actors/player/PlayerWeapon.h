#pragma once

#include "Irufemi.h"
#include "Resource/Audio/Se.h"
#include <memory>
#include <vector>
#include <cstdlib>

class Camera;
class ParticleSystem;
class IrufemiEngine;
class ModelRegion;

/**
 * @struct MissileData
 * @brief 誘導ミサイルのデータ
 */
struct MissileData {
    Vector3 position; // 現在位置
    Vector3 velocity; // 移動速度（ベクトル）
    Vector3 target;   // 誘導先の目標地点
    bool isActive;    // 飛んでいるか
    int timer;        // 生存フレーム数
};

/**
 * @struct MachineGunBullet
 * @brief 機関銃の弾データ
 */
struct MachineGunBullet {
    Vector3 position; // 現在位置
    Vector3 velocity; // 移動速度（ベクトル）
    bool isActive;    // 飛んでいるか
    int timer;        // 生存フレーム数
};

/**
 * @struct Cartridge
 * @brief 機関銃の薬莢データ
 */
struct Cartridge {
    Vector3 position;
    Vector3 velocity;
    Vector3 rotation;
    Vector3 angularVelocity; // 回転速度（くるくる回らせるため）
    bool isActive;
    int timer;
};

class PlayerWeapon {
public:
    PlayerWeapon() = default;
    ~PlayerWeapon() = default;

    void Initialize();

    // 毎フレームの更新（振動、弾の移動、薬莢の物理挙動など）
    void Update(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos, const Vector3& playerScale, bool isKarakuriCharged);

    /**
     * @brief 死亡演出中など、武器の発射制御をスキップしてパーティクルや弾の移動・寿命更新のみを行う
     */
    void UpdateParticlesOnly();

    // 描画処理 (viewMode は Player::ViewMode の値を int にキャストして受け取る。0=一人称)
    void Draw(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos, int viewMode, bool isBlinking, bool isDead);
    void DrawParticles(IrufemiEngine* engine);

    // スキル発動・停止
    void FireMissileSkill(const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& targetPos);
    void StartMachineGunSkill();
    void StopMachineGunSkill();
    void ClearMissiles();

    // 機関銃の残弾ゲッター（UIなどで使用）
    int GetMachineGunAmmo() const { return machineGunAmmo_; }
    int GetMaxMachineGunAmmo() const { return kMaxMachineGunAmmo; }
    bool IsMachineGunFiring() const { return machineGunActiveTimer_ > 0; }

    // 振動（シェイク）の値を取得（プレイヤー本体やカメラを揺らすため）
    const Vector3& GetMachineGunVibration() const { return machineGunVibration_; }
    const Vector3& GetMissileVibration() const { return missileVibration_; }
    


    // ImGui で調整するためのポインタ取得
    float* GetMachineGunVibrationScalePtr() { return &machineGunVibrationScale_; }
    float* GetMissileVibrationScalePtr() { return &missileVibrationScale_; }

    // デバッグ描画などで弾の情報を取得するため
    MachineGunBullet* GetMachineGunBullets() { return bullets_; }
    static int GetMaxMachineGunBullets() { return kMaxBullets; }
    MissileData* GetMissiles() { return missiles_; }
    static int GetMaxMissiles() { return kMaxMissiles; }

private:
    void UpdateMissile(const Vector3& targetPos, const Vector3& playerScale, bool isKarakuriCharged);
    void UpdateMachineGun(const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos);
    void FireMachineGunBullet(const Vector3& startPos, const Vector3& playerTranslate, const Vector3& playerRotate, float cameraPitch, const Vector3& targetPos);
    void UpdateCartridges();
    void EjectCartridge(const Vector3& startPos, bool isRight, const Vector3& playerTranslate, const Vector3& playerRotate, const Vector3& targetPos);

private:
    Camera* camera_ = nullptr;

    // --- 振動（シェイク）パラメータ ---
    Vector3 machineGunVibration_ = { 0.0f, 0.0f, 0.0f };
    Vector3 missileVibration_ = { 0.0f, 0.0f, 0.0f };
    int missileVibrationTimer_ = 0;
    const int kMissileVibrationDuration = 30; // ミサイル振動の持続フレーム
    float machineGunVibrationScale_ = 0.05f;
    float missileVibrationScale_ = 0.4f;

    // --- 機関銃・弾用オブジェクトとデータ ---
    inline static const Vector3 kMachineGunModelSize = { 6.0f, 1.6f, 6.0f };
    inline static const Vector3 kMachineGunScale = { 0.1f, 0.1f, 0.3f };
    // --- 大砲スケール（均一スケールで大きく表示） ---
    inline static const Vector3 kCannonScale = { 0.22f, 0.22f, 0.22f };
    // --- ロケットランチャースケール ---
    inline static const Vector3 kRocketLauncherScale = { 0.22f, 0.22f, 0.22f };

    std::unique_ptr<ParticleSystem> muzzleSmokeLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleSmokeRight_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashRight_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashAddLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashAddRight_ = nullptr;
    std::unique_ptr<ParticleSystem> missileFire_ = nullptr;
    std::unique_ptr<ParticleSystem> missileSmoke_ = nullptr;
    std::unique_ptr<ParticleSystem> bulletTrail_ = nullptr;
    std::unique_ptr<ParticleSystem> ejectionMistLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> ejectionMistRight_ = nullptr;

    std::unique_ptr<ObjClass> cannonObj_ = nullptr;
    std::unique_ptr<ObjClass> cannonObjRight_ = nullptr;

    std::unique_ptr<ObjClass> rocketLauncherObj_ = nullptr;
    std::unique_ptr<ObjClass> rocketLauncherObjRight_ = nullptr;

    bool isKarakuriCharged_ = false;

    static const int kMaxBullets = 100;
    std::unique_ptr<ModelRegion> bulletRegion_;
    MachineGunBullet bullets_[kMaxBullets] = {};

    int machineGunActiveTimer_ = 0;
    int machineGunFireTimer_ = 0;

    std::unique_ptr<Se> seShooting_;
    std::unique_ptr<Se> seMissileShot_;

    // --- 機関銃 弾薬（アモ）システム ---
    static const int kMaxMachineGunAmmo = 60; // 最大弾薬数（60連射分）
    int machineGunAmmo_ = kMaxMachineGunAmmo;  // 現在の残弾
    int machineGunAmmoRecoveryTimer_ = 0;      // 回復間隔タイマー
    int machineGunRecoveryCooldown_ = 0;       // 自動回復開始までのクールタイム
    static const int kAmmoRecoveryInterval = 30; // 何フレームに1発回復するか

    // --- 薬莢（Cartridge）用オブジェクトとデータ ---
    static const int kMaxCartridges = 100;
    std::unique_ptr<ModelRegion> cartridgeRegion_;
    Cartridge cartridges_[kMaxCartridges] = {};
    const float kGravity = 0.02f;

    // --- 誘導ミサイル用 ---
    // ★修正: 16発に戻す
    static const int kMaxMissiles = 16;
    std::unique_ptr<ModelRegion> missileRegion_;
    MissileData missiles_[kMaxMissiles] = {};
    const float kMissileSpeed = 0.8f;

    float missileTurnSpeedNormal_ = 0.04f;
    float missileTurnSpeedCharged_ = 0.08f;
    float missileSpreadMagnitudeBase_ = 1.5f;
    float missileSpreadMagnitudeRand_ = 1.5f;

public:
    float* GetMissileTurnSpeedNormalPtr() { return &missileTurnSpeedNormal_; }
    float* GetMissileTurnSpeedChargedPtr() { return &missileTurnSpeedCharged_; }
    float* GetMissileSpreadMagnitudeBasePtr() { return &missileSpreadMagnitudeBase_; }
    float* GetMissileSpreadMagnitudeRandPtr() { return &missileSpreadMagnitudeRand_; }
    float* GetRocketLauncherChargedScaleMultiplierPtr() { return &rocketLauncherChargedScaleMultiplier_; }
    float* GetRocketLauncherYOffsetPtr() { return &rocketLauncherYOffset_; }

private:
    float rocketLauncherChargedScaleMultiplier_ = 1.4f; // チャージ強化時のミサイルランチャースケール倍率
    float rocketLauncherYOffset_ = 0.2f; // チャージ強化時のミサイルランチャーのY軸オフセット

};