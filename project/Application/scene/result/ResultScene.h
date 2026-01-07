    #pragma once

#include "scene/IScene.h"
#include <memory>
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

class ResultScene : public IScene {
public:
    ~ResultScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // 音源
    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

private:
    IrufemiEngine* engine_ = nullptr;

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;

    bool debugMode = false;

    // 結果表示用スプライト
    std::unique_ptr<Sprite> resultImage_ = nullptr;
    std::unique_ptr<Sprite> continueText_ = nullptr;

    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;

    // 演出用タイマー
    float blinkTimer_ = 0.0f;
};
