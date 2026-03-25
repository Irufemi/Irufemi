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