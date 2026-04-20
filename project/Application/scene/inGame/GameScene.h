#pragma once

#include "Framework/IScene.h"
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

class GameScene : public IScene {
public:
    GameScene();
    ~GameScene() override;

    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    void PauseUpdate() override;
    void PauseDraw() override;
    bool IsPausable() const override { return true; }
    void DrawDebugTab() override;

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

    // 敵被ダメージ
    static constexpr int kDamageMeleeToEnemy = 20;           ///< 近接攻撃
    static constexpr int kDamageMachineGunToEnemy = 10;      ///< マシンガン
    static constexpr int kDamageMissileToEnemy = 200;        ///< ミサイル
    static constexpr int kDamageProjectilePartToEnemy = 500; ///< 部位同士の衝突

    // 建物被ダメージ
    static constexpr int kDamageMeleeToBuilding = 30;         ///< 近接攻撃→建物
    static constexpr int kDamageMachineGunToBuilding = 5;     ///< マシンガン→建物
    static constexpr int kDamageMissileToBuilding = 100;      ///< ミサイル→建物
    static constexpr int kDamagePartToBuilding = 50;          ///< 飛んだ部位→建物
    static constexpr int kDamageFlyingBuildingToEnemy = 300;   ///< 飛んだ建物→敵
    static constexpr int kDamageFlyingBuildingToBuilding = 80; ///< 飛んだ建物→建物

    // 当たり判定(半径)
    static constexpr float kMachineGunBulletRadius = 1.0f;   ///< マシンガン
    static constexpr float kMissileRadius = 2.0f;            ///< ミサイル

    // エフェクト・物理演算
    static constexpr float kMeleeEffectSizeMultiplier = 1.5f;   ///< 近接攻撃エフェクト倍率
    static constexpr float kMeleeScatterSpeedMultiplier = 1.0f;  ///< 近接攻撃の部位吹き飛び初速倍率
    static constexpr float kCollisionScatterMultiplier = -0.5f; ///< 衝突時の反発係数
    static constexpr float kMathEpsilon = 0.001f;               ///< ゼロ除算防止用微小値

    // --- デバッグUI定数 ---
    
    static constexpr float kDebugCameraDragSpeed = 0.1f;    ///< カメラ距離スライダー変化量
    static constexpr float kDebugCameraDistMin = 1.0f;      ///< カメラ最小距離
    static constexpr float kDebugCameraDistMax = 1000.0f;   ///< カメラ最大距離

    // --------------------------------

    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<Camera> camera_ = nullptr;
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode_ = false;
    bool isFirstDebug_ = true;

    // ゲームオブジェクト
    std::unique_ptr<Player> player_ = nullptr;
    std::unique_ptr<Enemy> boss_ = nullptr;
    std::unique_ptr<Field> field_ = nullptr;
    std::unique_ptr<Skydome> skydome_ = nullptr;

    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
    
    // 当たり判定の有効化フラグ
    bool isCollisionEnabled_ = true;

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

    /**
     * @brief プレイヤーと敵の座標に基づき、動的なポイントライトのパラメータを計算・更新する
     */
    void UpdateDynamicLights();

    /**
     * @brief カメラとフレームデータの更新
     */
    void UpdateCameraAndFrameData();
};