#pragma once

#include "scene/IScene.h"

#include <memory>
#include <vector>

class IrufemiEngine;

class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;
struct AreaLight;

#include "2D/Sprite.h"
#include "audio/Se.h"
#include "audio/Bgm.h"

#include "contents/Effect/Fade.h" 

/**
 * @class SelectScene
 * @brief ステージ選択画面を管理するクラス
 *
 * プレイヤーがプレイするステージを選択し、決定に応じてゲームシーンへ遷移します。
 */
class SelectScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~SelectScene() override;

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

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    // 選択シーンの状態
    enum class Phase {
        Selecting,  // 選択中
        Confirming, // 決定演出中
        FadingOut,  // フェードアウト中
    };

    // UIスプライト
    std::unique_ptr<Sprite> text_title_ = nullptr;
    std::unique_ptr<Sprite> text_1_ = nullptr;
    std::unique_ptr<Sprite> text_2_ = nullptr;
    std::vector<Sprite*> stageSprites_; // ステージ選択肢をまとめて管理

    // 状態管理
    Phase phase_ = Phase::Selecting;
    int currentStageIndex_ = 0; // 0: ステージ1, 1: ステージ2

    // 演出用タイマー
    float blinkTimer_ = 0.0f;
    float confirmationTimer_ = 0.0f;

    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;
    
    // 音源
    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

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
};
