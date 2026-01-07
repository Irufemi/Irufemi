#pragma once

#include "scene/IScene.h"
#include <memory>
#include <vector>
#include "contents/Effect/Fade.h" // Fadeをインクルード
#include "audio/Se.h"
#include "audio/Bgm.h"

class IrufemiEngine;
class DebugCamera;
class Camera;
class Sprite; // Spriteの前方宣言
struct PointLight;
struct SpotLight;
struct DirectionalLight;

class SelectScene : public IScene {
public:
    ~SelectScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // 音源
    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

private:
    // 選択シーンの状態
    enum class Phase {
        Selecting,  // 選択中
        Confirming, // 決定演出中
        FadingOut,  // フェードアウト中
    };

    IrufemiEngine* engine_ = nullptr;

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    bool debugMode = false;

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
};
