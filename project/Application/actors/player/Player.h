#pragma once

#include "Irufemi.h"
#include "Engine/Core/Math/Geometry/OBB.h" // OBBを追加
#include <memory>
#include <vector>

// 前方宣言
class Camera;
class Line3DRegion;
class Enemy; // ★追加：Enemyクラスを前方宣言して、ポインタを使えるようにする

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
    void Initialize(InputManager* input, Camera* camera, IrufemiEngine* engine, Mouse* mouse);

    /**
     * @brief 更新処理
     */
    void Update();

    /**
     * @brief 描画処理
     */
    void Draw();

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
    bool IsDead() const { return isDead_; }

    /**
     * @brief 敵の座標を受け取る関数（オートエイム用）
     * @param targetPos 敵の座標
     */
    void SetTargetPosition(const Vector3& targetPos) { targetPos_ = targetPos; }

    /**
     * @brief 敵を吹き飛ばす命令を受け取る関数（★追加）
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
     * @brief ミサイル攻撃処理（複数・誘導対応）
     */
    void HandleMissile();

    /**
     * @brief 機関銃の処理（オートエイム連射）
     */
    void HandleMachineGun();

    /**
     * @brief 機関銃の弾を1発発射する処理
     * @param startPos 発射位置（肩の位置）
     */
    void FireMachineGunBullet(const Vector3& startPos);

    /**
     * @brief カメラ座標の更新
     */
    void UpdateCamera();

private:
    // 外部依存
    InputManager* input_ = nullptr;
    Camera* camera_ = nullptr;
    IrufemiEngine* engine_ = nullptr;
    Mouse* mouse_ = nullptr;

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
    std::unique_ptr<ObjClass> machineGunObjLeft_ = nullptr;
    std::unique_ptr<ObjClass> machineGunObjRight_ = nullptr;

    static const int kMaxBullets = 100;
    std::unique_ptr<ObjClass> bulletObjs_[kMaxBullets];
    MachineGunBullet bullets_[kMaxBullets];

    int machineGunActiveTimer_ = 0;
    int machineGunFireTimer_ = 0;
    Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };

    // --- 誘導ミサイル用 ---
    static const int kMaxMissiles = 4;
    std::unique_ptr<ObjClass> missileObjs_[kMaxMissiles];
    MissileData missiles_[kMaxMissiles];
    int missileCooldown_ = 0;
    const float kMissileSpeed = 0.8f;

    // トランスフォーム
    Vector3 scale_ = { 0.3f, 1.0f, 0.3f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, -50.0f };

    // 移動用物理変数
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    ViewMode viewMode_ = ViewMode::kThirdPerson;
    bool isGrounded_ = true;

    // --- 近接攻撃判定用 ---
    enum class AttackState {
        kNone,      // 待機
        kCharging,  // チャージ中
        kAttacking  // 攻撃中
    };
    AttackState attackState_ = AttackState::kNone;
    int chargeTimer_ = 0;            // ボタンを長押ししているフレーム数
    float currentChargeRate_ = 0.0f; // スイング時に保持するチャージ倍率

    AttackCollision attackCollision_ = { {0.0f, 0.0f, 0.0f}, 4.0f, false };
    int attackActiveTimer_ = 0;
    const int kAttackDuration = 20;  // スイングにかかるフレーム数

    // --- ステータス・やられ判定用 ---
    int hp_ = 100;
    bool isDead_ = false;
    int invincibleTimer_ = 0;
    const float kColliderRadius = 1.0f;

    // --- ノックバック（吹き飛ばし）処理用変数（★追加） ---
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