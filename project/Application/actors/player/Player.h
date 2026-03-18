#pragma once

#include "Irufemi.h"
#include "Engine/Core/Math/Geometry/OBB.h" // OBBを追加
#include <memory>
#include <vector>

// 前方宣言
class Camera;
class Line3DRegion;
class ParticleSystem;
class Enemy; // Enemyクラスを前方宣言して、ポインタを使えるようにする

/**
 * @struct AttackCollision
 * @brief プレイヤーの攻撃判定（攻撃を当てる側）データ
 */
struct AttackCollision {
    Vector3 center; // 判定の中心座標
    float radius;   // 判定の半径
    bool isActive;  // 攻撃判定が有効か
};

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

/**
 * @struct PlayerCollider
 * @brief プレイヤー自身の当たり判定（攻撃を受ける側）データ
 */
struct PlayerCollider {
    Vector3 center; // 判定の中心座標
    float radius;   // 判定の半径
    OBB obb;        // OBBの当たり判定データを追加
};

/**
 * @class Player
 * @brief プレイヤーキャラクターを管理するクラス
 */
class Player {
public:
    enum class ViewMode {
        kFirstPerson, // 一人称
        kThirdPerson  // 三人称
    };

    // デストラクタ
    ~Player();

    /**
     * @brief 初期化処理
     * @param input InputManagerのポインタ
     * @param camera Cameraのポインタ
     * @param engine IrufemiEngineのポインタ
     * @param mouse Mouseのポインタ
     */
    void Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine);

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画処理
     */
    void Draw();
    void DrawParticles(); // ★追加：パーティクルのみを後で描画するためのメソッド

    // ゲッター
    const Vector3& GetTranslate() const { return translate_; }
    const Vector3& GetRotate() const { return rotate_; }

    /**
     * @brief 攻撃判定の取得（敵に攻撃を当てるときに使う）
     * @return 攻撃判定構造体
     */
    const AttackCollision& GetAttackCollision() const { return attackCollision_; }

    /**
     * @brief マシンガンの弾の取得
     */
    MachineGunBullet* GetMachineGunBullets() { return bullets_; }
    static int GetMaxMachineGunBullets() { return kMaxBullets; }

    /**
     * @brief ミサイルの取得
     */
    MissileData* GetMissiles() { return missiles_; }
    static int GetMaxMissiles() { return kMaxMissiles; }

    /**
     * @brief やられ判定の取得（敵からの攻撃を受けるときに使う）
     * @return やられ判定構造体
     */
    PlayerCollider GetCollider() const;

    /**
     * @brief ダメージを受ける処理
     * @param damage 受けるダメージ量
     */
    void ApplyDamage(int damage);

    // HPと生存状態の取得
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return kMaxHp; } // 最大HP取得用
    bool IsDead() const { return isDead_; }

    /**
     * @brief 敵の座標を受け取る関数（オートエイム用）
     * @param targetPos 敵の座標
     */
    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }

    /**
     * @brief 敵を吹き飛ばす命令を受け取る関数
     * @param enemy 吹き飛ばす対象のEnemyポインタ
     */
    void HitAndKnockback(Enemy* enemy);

private:
    /**
     * @brief 移動処理
     */
    void HandleMovement();

    /**
     * @brief 攻撃処理
     */
    void HandleAttack();

    /**
     * @brief スキル・からくりチャージの入力処理
     */
    void HandleSkill();

    /**
     * @brief ミサイルスキル発動
     */
    void FireMissileSkill();

    /**
     * @brief 機関銃スキル発動
     */
    void StartMachineGunSkill();

    /**
     * @brief ミサイル攻撃の更新（移動・誘導）
     */
    void UpdateMissile();

    /**
     * @brief 機関銃の更新（連射・弾の移動）
     */
    void UpdateMachineGun();

    /**
     * @brief 機関銃の弾を1発発射する処理
     * @param startPos 発射位置（肩の位置）
     */
    void FireMachineGunBullet(const Vector3& startPos);

    /**
     * @brief カメラ座標の更新
     */
    void UpdateCamera();

    /**
     * @brief 薬莢の更新処理（移動、重力、回転）
     */
    void UpdateCartridges();

    /**
     * @brief 薬莢を1つ排出する処理
     * @param startPos 排出位置（肩の位置）
     * @param isRight 右側の銃かどうか（排出方向を決めるため）
     */
    void EjectCartridge(const Vector3& startPos, bool isRight);

private:
    // 外部依存
    InputManager* input_ = nullptr;
    Camera* camera_ = nullptr;
    IrufemiEngine* engine_ = nullptr;
    // Mouse* mouse_ = nullptr; // InputManager 経由で取得するため削除

    // --- カメラ・マウス操作用パラメータ ---
    float mouseSensitivity_ = 5.0f;           // マウス感度
    float mouseSensitivityMultiplier_ = 1.0f; // マウス感度の倍率
    float cameraPitch_ = -0.1f;               // カメラの上下の角度（ピッチ）
    bool isCameraControlEnabled_ = true;      // カメラ操作の有効/無効フラグ

    // 3Dモデル本体
    std::unique_ptr<ObjClass> obj_ = nullptr;
    // 攻撃可視化用モデル（分身）
    std::unique_ptr<ObjClass> attackObj_ = nullptr;

    // --- 一人称視点用マスク画像スプライト ---
    std::unique_ptr<Sprite> maskSprite_ = nullptr;

    // --- 機関銃・弾用オブジェクトとデータ ---
    inline static const Vector3 kMachineGunModelSize = { 6.0f, 1.6f, 6.0f };
    inline static const Vector3 kMachineGunScale = { 0.1f, 0.1f, 0.3f };

    std::unique_ptr<ParticleSystem> muzzleSmokeLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleSmokeRight_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashLeft_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashRight_ = nullptr;
    std::unique_ptr<ParticleSystem> muzzleFlashAddLeft_ = nullptr; // ★追加：加算合成用
    std::unique_ptr<ParticleSystem> muzzleFlashAddRight_ = nullptr; // ★追加：加算合成用
    std::unique_ptr<ParticleSystem> missileFire_ = nullptr;
    std::unique_ptr<ParticleSystem> missileSmoke_ = nullptr;
    std::unique_ptr<ObjClass> machineGunObjLeft_ = nullptr;
    std::unique_ptr<ObjClass> machineGunObjRight_ = nullptr;

    static const int kMaxBullets = 100;
    std::unique_ptr<ObjClass> bulletObjs_[kMaxBullets];
    MachineGunBullet bullets_[kMaxBullets] = {};

    int machineGunActiveTimer_ = 0;
    int machineGunFireTimer_ = 0;
    Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };

    // --- 薬莢（Cartridge）用オブジェクトとデータ ---
    static const int kMaxCartridges = 100;
    std::unique_ptr<ObjClass> cartridgeObjs_[kMaxCartridges];
    Cartridge cartridges_[kMaxCartridges] = {};

    // --- 誘導ミサイル用 ---
    static const int kMaxMissiles = 4;
    std::unique_ptr<ObjClass> missileObjs_[kMaxMissiles];
    MissileData missiles_[kMaxMissiles] = {};
    const float kMissileSpeed = 0.8f;

    // --- ★スキルとからくりチャージ用 ---
    int skillDurationTimer_ = 0;             // スキル実行中（打ち終わるまで）の時間
    int skillCooldownTimer_ = 0;             // スキルのクールタイム
    const int kSkillCooldownTime = 300;      // クールタイム5秒（60FPS想定）

    int karakuriChargeTimer_ = 0;            // からくりチャージ用の長押し時間
    const int kKarakuriChargeTime = 300;     // チャージ完了までの時間（5秒）
    bool isKarakuriCharged_ = false;         // チャージ状態（界王拳状態）

    int karakuriActiveTimer_ = 0;            // ★追加：界王拳状態の持続時間
    const int kKarakuriActiveTime = 1200;    // ★追加：20秒間（60FPS * 20 = 1200フレーム）

    // トランスフォーム
    Vector3 scale_ = { 0.3f, 1.0f, 0.3f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, -50.0f };

    // 移動用物理変数
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    ViewMode viewMode_ = ViewMode::kThirdPerson;
    bool isGrounded_ = true;

    // --- ★追加：回避用変数 ---
    int dodgeCooldownTimer_ = 0;         // 回避のクールタイム
    const int kDodgeCooldownTime = 120;  // クールタイム2秒（60FPS想定）
    int dodgeDurationTimer_ = 0;         // 回避行動自体の持続時間
    const int kDodgeDurationTime = 20;   // 回避時間（約0.3秒）
    Vector3 dodgeDirection_ = { 0.0f, 0.0f, 0.0f }; // 回避する方向
    const float kDodgeSpeed = 0.6f;      // 回避の移動速度

    // --- 近接攻撃判定用 ---
    enum class AttackState {
        kNone,      // 待機
        kCharging,  // チャージ中
        kAttacking  // 攻撃中
    };
    AttackState attackState_ = AttackState::kNone;
    int chargeTimer_ = 0;            // ボタンを長押ししているフレーム数
    float currentChargeRate_ = 0.0f; // スイング時に保持するチャージ倍率

    AttackCollision attackCollision_ = {};
    int attackActiveTimer_ = 0;
    const int kAttackDuration = 20;  // スイングにかかるフレーム数

    // --- ステータス・やられ判定用 ---
    const int kMaxHp = 100;           // 最大体力
    int hp_ = kMaxHp;                 // 現在の体力
    bool isDead_ = false;
    int invincibleTimer_ = 0;
    const float kColliderRadius = 1.0f;

    // --- ノックバック（吹き飛ばし）処理用変数 ---
    Enemy* knockbackTarget_ = nullptr;            // 吹き飛ばしている最中の敵
    Vector3 knockbackVelocity_ = { 0.0f, 0.0f, 0.0f }; // 吹き飛ぶ速度
    int knockbackTimer_ = 0;                      // 吹き飛ぶ時間（フレーム）

    // パラメータ
    const float kMoveSpeed = 0.2f;
    const float kJumpForce = 0.25f;
    const float kGravity = 0.02f;

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> lineOBB_ = nullptr;
    bool isDebugDrawOBB_ = false;
#endif

    // フィールドの境界
    const float kFieldRangeX = 100.0f;
    const float kFieldRangeZ = 100.0f;
};
