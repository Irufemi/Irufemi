#pragma once

#include "scene/IScene.h"

#include "audio/Bgm.h"
#include "audio/Se.h"
#include "math/shape/LinePrimitive.h"
#include "2D/Sprite.h"
#include "3D/ObjClass.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "3D/PointLightClass.h"
#include "3D/SpotLightClass.h"
#include "3D/CylinderClass.h"
#include "contents/Effect/Fade.h"
#include <memory>
#include <vector>

class Camera;
class DebugCamera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {
private: // 音源
    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

private: // 描画物
    // タイトル(アンナイトのア)
    std::unique_ptr<ObjClass> text_a_ = nullptr;
    // タイトル(アンナイトのン)
    std::unique_ptr<ObjClass> text_n_ = nullptr;
    // タイトル(アンナイトのナ)
    std::unique_ptr<ObjClass> text_na_ = nullptr;
    // タイトル(アンナイトのイ)
    std::unique_ptr<ObjClass> text_i_ = nullptr;
    // タイトル(アンナイトのト)
    std::unique_ptr<ObjClass> text_to_ = nullptr;
    // プッシュキー
    std::unique_ptr<Sprite> text_pushKey_ = nullptr;

    // フェード
    std::unique_ptr<Fade> fade_ = nullptr;

    bool isChangingScene_ = false;

    bool debugMode = false;

    // --- アニメーション用変数 ---
    float animationTimer_ = 0.0f; // アニメーションのタイマー
    float floatAmplitude_ = 0.1f; // 上下の揺れの振幅
    float floatSpeed_ = 1.5f;     // 揺れの速さ
    std::vector<Vector3> initialTextPositions_; // 各文字の初期位置

    // --- プッシュキーアニメーション用変数 ---
    enum class PushKeyState {
        NormalBlink, // 通常明滅
        FastBlink,   // 高速明滅
        Done         // 完了
    };
    PushKeyState pushKeyState_ = PushKeyState::NormalBlink;
    float pushKeyAnimTimer_ = 0.0f; // プッシュキー用タイマー


public: // メンバ関数
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ変数
    IrufemiEngine* engine_ = nullptr;

    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;

    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;

    std::unique_ptr<PointLight> pointLight_ = nullptr;

    std::unique_ptr<SpotLight> spotLight_ = nullptr;
};