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

    // システム・構成
    // 敵パーツ数
    static constexpr int kEnemyBodyPartsCount = 3;
    // デバッグカメラ切替キー('P')
    static constexpr int kKeyDebugCameraToggle = 0x19;

    // プレイヤー被ダメージ
    // ビーム被弾
    static constexpr int kDamageBeamToPlayer = 10;
    // 敵部位接触
    static constexpr int kDamagePartToPlayer = 10;

    // 敵被ダメージ
    // 近接攻撃
    static constexpr int kDamageMeleeToEnemy = 20;
    // マシンガン
    static constexpr int kDamageMachineGunToEnemy = 10;
    // ミサイル
    static constexpr int kDamageMissileToEnemy = 200;
    // 部位同士の衝突
    static constexpr int kDamageProjectilePartToEnemy = 500;

    // 当たり判定(半径)
    // マシンガン
    static constexpr float kMachineGunBulletRadius = 1.0f;
    // ミサイル
    static constexpr float kMissileRadius = 2.0f;

    // エフェクト・物理演算
    // 近接攻撃エフェクト倍率
    static constexpr float kMeleeEffectSizeMultiplier = 1.5f;
    // 近接攻撃の部位吹き飛び初速倍率
    static constexpr float kMeleeScatterSpeedMultiplier = 1.0f;
    // 衝突時の反発係数
    static constexpr float kCollisionScatterMultiplier = -0.5f;
    // ゼロ除算防止用微小値
    static constexpr float kMathEpsilon = 0.001f;

    // デバッグUI
    // カメラ距離スライダー変化量
    static constexpr float kDebugCameraDragSpeed = 0.1f;
    // カメラ最小距離
    static constexpr float kDebugCameraDistMin = 1.0f;
    // カメラ最大距離
    static constexpr float kDebugCameraDistMax = 1000.0f;

    // --------------------------------

    IrufemiEngine* engine_ = nullptr;
    std::unique_ptr<Camera> camera_ = nullptr;
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;
    bool debugMode_ = false;
    bool isFirstDebug_ = true;

    // マウスは InputManager から取得するため削除

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

    // カメラとフレームデータの更新ロジックを共通化
    void UpdateCameraAndFrameData();
};