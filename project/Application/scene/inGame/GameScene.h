#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

#include "3D/ObjClass.h"

#include "contents/player/Player.h"
#include "contents/healer/Healer.h"
#include "contents/enemy/Enemy.h"
#include "contents/wall/Wall.h"

/// <summary>
/// ゲーム
/// </summary>
class GameScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~GameScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;
    // ポーズ中の更新
    void PauseUpdate() override;
    // ポーズ中の描画
    void PauseDraw() override;
    // このシーンがポーズ可能かを返す
    bool IsPausable() const override { return true; }

private: // メンバ関数(内部ヘルパ)

    void CollisionCheck();

private: // メンバ変数(ゲーム)

    std::list<Wall*> walls_;
    static inline const int32_t kMaxWall_ = 16;

    std::list<Enemy*> enemies_;
    static inline const int32_t kMaxEnemy_ = 5;

    std::unique_ptr<Player> player_ = nullptr;

    std::unique_ptr<Healer> healer_ = nullptr;

    Transform worldTransform_;

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool debugMode_ = false;
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    // ポーズ表示用
    std::unique_ptr<Sprite> pauseSprite_ = nullptr;
};