#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>
#include "actors/enemy/Enemy.h"

// 前方宣言
class IrufemiEngine;
class InputManager;
class Camera;
class DebugCamera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;


/**
 * @class GameScene
 * @brief ゲームプレイ中のメインシーンを管理するクラス
 *
 * プレイヤー、敵、マップなどのゲームオブジェクトを管理し、
 * ゲームのロジック、更新、描画、衝突判定などを担当します。
 */
class GameScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~GameScene() override;
    /**
     * @brief 初期化処理
     * @param engine IrufemiEngineのポインタ
     */
    void Initialize(IrufemiEngine* engine) override;
    /**
     * @brief 毎フレームの更新処理
     */
    void Update() override;
    /**
     * @brief 描画処理
     */
    void Draw() override;
    // ポーズ中の更新
    void PauseUpdate() override;
    // ポーズ中の描画
    void PauseDraw() override;
    // このシーンがポーズ可能かを返す
    bool IsPausable() const override { return true; }

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

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
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;

    // ポーズ表示用
    std::unique_ptr<Sprite> pauseSprite_ = nullptr;

    // ボス
    std::unique_ptr<Enemy> boss_ = nullptr;
};