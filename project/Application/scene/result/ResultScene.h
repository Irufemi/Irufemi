    #pragma once

#include "scene/IScene.h"
#include <memory>

class IrufemiEngine;
class DebugCamera;
class Camera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

#include "audio/Se.h"
#include "audio/Bgm.h"
#include "2D/Sprite.h"

#include "contents/Effect/Fade.h"

class ResultScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)

    // 結果表示用スプライト
    std::unique_ptr<Sprite> resultImage_ = nullptr;
    std::unique_ptr<Sprite> continueText_ = nullptr;

    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;

    // 演出用タイマー
    float blinkTimer_ = 0.0f;
    
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
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;
};
