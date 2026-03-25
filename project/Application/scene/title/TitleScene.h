#pragma once

#include "Framework/IScene.h"

#include <memory>
#include <vector>

class IrufemiEngine;
class Camera;
class DebugCamera;
class Sprite;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

/**
 * @class TitleScene
 * @brief タイトル画面を管理するクラス
 *
 * ゲームの開始をユーザーに促し、入力に応じてステージ選択シーンへ遷移します。
 * タイトルロゴのアニメーションやBGMの再生も担当します。
 */
class TitleScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~TitleScene() override;

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
    void DrawDebugTab() override;


private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    // シーン表示仮置きスプライト
    std::unique_ptr<Sprite> sampleSprite_ = nullptr;

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool isChangingScene_ = false;

    bool debugMode_ = false;
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};