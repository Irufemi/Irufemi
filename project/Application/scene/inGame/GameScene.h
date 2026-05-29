#pragma once

#include "Framework/BaseScene.h"
#include "Irufemi.h"
#include <memory>
#include <vector>

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Player;
class Enemy;
class Field;
class Skydome;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;
class Mouse; // ★追加：Mouseの前方宣言
class Building;
class EnemyHPBar;
class EnemyPartHPBar;
class Sprite;
class DynamicArenaLight;

class GameScene : public BaseScene {
public:
    GameScene();
    ~GameScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    bool IsCursorVisible() const override { return false; }
    void DrawDebugTab() override;

    // 現在のシーンがホワイトアウト（爆散による白飛び等）の文脈にあるかを判定
    bool IsWhiteoutContext() const;

    static float GetClearTime() { return clearTime_; }

private:

    // --- システム・構成定数 ---
    
    // 敵パーツ数
    static constexpr int kEnemyBodyPartsCount = 3;
    // デバッグカメラ切替キー('P')
    static constexpr int kKeyDebugCameraToggle = 0x19;

    // --- 当たり判定定数 ---

    // プレイヤー被ダメージ
    static constexpr int kDamageBeamToPlayer = 10;           ///< ビーム被弾
    static constexpr int kDamagePartToPlayer = 10;           ///< 敵部位接触
    static constexpr int kDamageTackleWaveToPlayer = 10;     ///< 突進の砂煙接触
    static constexpr int kDamageCrashWaveToPlayer = 30;      ///< 壁激突の大爆発接触
    static constexpr int kDamageBombToPlayer = 10;           ///< ボム爆発接触

    // 敵被ダメージ
    static constexpr int kDamageMeleeToEnemy = 20;           ///< 近接攻撃
    static constexpr int kDamageMachineGunToEnemy = 3;      ///< マシンガン
    static constexpr int kDamageMissileToEnemy = 50;        ///< ミサイル
    static constexpr int kDamageProjectilePartToEnemy = 250; ///< 部位同士の衝突

    // 建物被ダメージ
    static constexpr int kDamageMeleeToBuilding = 16;         ///< 近接攻撃→建物
    static constexpr int kDamageMachineGunToBuilding = 6;     ///< マシンガン→建物
    static constexpr int kDamageMissileToBuilding = 100;      ///< ミサイル→建物
    static constexpr int kDamagePartToBuilding = 50;          ///< 飛んだ部位→建物
    static constexpr int kDamageFlyingBuildingToEnemy = 200;   ///< 飛んだ建物→敵
    static constexpr int kDamageFlyingBuildingToBuilding = 50; ///< 飛んだ建物→建物

    // 当たり判定(半径)
    static constexpr float kMachineGunBulletRadius = 1.0f;   ///< マシンガン
    static constexpr float kMissileRadius = 2.0f;            ///< ミサイル

    // エフェクト・物理演算
    static constexpr float kMeleeEffectSizeMultiplier = 1.5f;   ///< 近接攻撃エフェクト倍率
    static constexpr float kMeleeScatterSpeedMultiplier = 1.0f;  ///< 近接攻撃の部位吹き飛び初速倍率
    static constexpr float kCollisionScatterMultiplier = -0.5f; ///< 衝突時の反発係数
    static constexpr float kMathEpsilon = 0.001f;               ///< ゼロ除算防止用微小値
    static constexpr float kBlowCollisionDelay = 0.2f;          ///< 吹き飛んだ部位が即時自爆衝突するのを防ぐクールタイム（秒）

    // --- デバッグUI定数 ---
    
    static constexpr float kDebugCameraDragSpeed = 0.1f;    ///< カメラ距離スライダー変化量
    static constexpr float kDebugCameraDistMin = 1.0f;      ///< カメラ最小距離
    static constexpr float kDebugCameraDistMax = 1000.0f;   ///< カメラ最大距離

    // --------------------------------

    bool isFirstDebug_ = true;
    bool isDebugCameraMode_ = false; // 仮で追加（元々無かったら）

    // 死亡演出カメラ用
    bool isDeathCameraMode_ = false;
    float deathCameraLerpTimer_ = 0.0f;
    Vector3 initialCameraPos_ = {};
    Vector3 initialCameraTarget_ = {};

    // --- 死亡演出カメラ・自動後退調整用定数（メンバー変数） ---
    static constexpr float kCameraBehindDistance = 32.0f;       ///< 死亡演出時にプレイヤーの背後へカメラを引く距離 (大幅引きに変更)
    static constexpr float kCameraHeightOffset = 2.0f;           ///< プレイヤー座標からのカメラの高さオフセット (低くして見上げアングルに変更)
    static constexpr float kGroundClampMinY = 1.2f;             ///< カメラが地面を突き抜けるのを防ぐ最小Y座標 (カメラを下げたのに対応)
    static constexpr float kBossLookAtHeightOffset = 9.0f;      ///< 死亡演出中のボスの注視点高さオフセット（注視点を上げて見上げ感を強調）
    static constexpr float kTargetPlayerBossDistance = 48.0f;   ///< 理想の対比レイアウトを作るためのプレイヤーとボスの基準距離 (引き対応)
    static constexpr float kPlayerBackoffSpeed = 15.0f;          ///< 近すぎるプレイヤーを理想距離へ滑らかに後退させる秒速速度

    // ゲームオブジェクト
    std::unique_ptr<Player> player_ = nullptr;
    std::unique_ptr<Enemy> boss_ = nullptr;
    // フィールド・スカイドーム
    std::unique_ptr<Field> field_ = nullptr;
    std::unique_ptr<Skydome> skydome_ = nullptr;
    
    // バトル用動的エリアライト
    std::unique_ptr<DynamicArenaLight> dynamicArenaLight_ = nullptr;
    
    std::unique_ptr<Sprite> uiLClickNormal_ = nullptr;
    std::unique_ptr<Sprite> uiLClickCharged_ = nullptr;
    std::unique_ptr<Sprite> uiRClickNormal_ = nullptr;
    std::unique_ptr<Sprite> uiRClickCharged_ = nullptr;
    std::unique_ptr<Sprite> uiE_ = nullptr;
    std::unique_ptr<Sprite> uiV_ = nullptr;
    std::unique_ptr<Sprite> uiSpace_ = nullptr;
    std::unique_ptr<Sprite> cooldownWarningSprite_ = nullptr;
    std::unique_ptr<Sprite> keyEscSprite_ = nullptr;

    // 当たり判定の有効化フラグ
    bool isCollisionEnabled_ = true;

    // ビルの自動生成タイマー
    float buildingSpawnTimer_ = 0.0f;

    // クリアタイム
    static float clearTime_;

    // --- 内部整理用メソッド ---

    /**
     * @brief 全ての当たり判定をチェックする
     */
    void CheckAllCollisions();

    /**
     * @brief プレイヤーから敵への当たり判定（近接、弾丸、ミサイル）
     */
    void CheckPlayerToEnemyCollisions();

    /**
     * @brief 敵からプレイヤーへの当たり判定（ビーム、スタンプ、部位接触）
     */
    void CheckEnemyToPlayerCollisions();

    /**
     * @brief 吹き飛んだ部位同士、または部位と敵の当たり判定
     */
    void CheckFlyingPartsCollisions();

    /**
     * @brief プレイヤーと建物の当たり判定（押し戻し＋攻撃）
     */
    void CheckPlayerBuildingCollisions();

    /**
     * @brief 敵（本体・ビーム・スタンプ・Bite）と建物の当たり判定
     */
    void CheckEnemyBuildingCollisions();

    /**
     * @brief 飛んだ部位と建物の当たり判定（建物にダメージ＋部位反射）
     */
    void CheckFlyingPartsBuildingCollisions();

    /**
     * @brief 飛んだ建物と敵部位の当たり判定（敵にダメージ＋建物爆散）
     */
    void CheckFlyingBuildingsVsEnemyCollisions();

    /**
     * @brief 飛んだ建物と他の建物の当たり判定
     */
    void CheckFlyingBuildingsVsBuildingsCollisions();
};