#pragma once

#include "scene/IScene.h"

#include <memory>

class IrufemiEngine;
class DebugCamera;
class Camera;
struct PointLight;
struct SpotLight;
struct DirectionalLight;

#include "3D/ObjClass.h"
#include "2D/Sprite.h"
#include "audio/Bgm.h"
#include "audio/Se.h"

#include "contents/Effect/Fade.h"

/// <summary>
/// タイトル
/// </summary>
class TitleScene : public IScene {
public: // メンバ関数(ゲーム)

public: // メンバ関数(システム)
    ~TitleScene() override;
    void Initialize(IrufemiEngine* engine) override;
    void Update() override;
    void Draw() override;

private: // メンバ関数(内部ヘルパ)

private: // メンバ変数(ゲーム)
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
    
    // 音源
    // bgm
    std::unique_ptr<Bgm> bgm_ = nullptr;
    // se(決定音)
    std::unique_ptr<Se> se_select_ = nullptr;

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
    std::unique_ptr<PointLight> pointLight_ = nullptr;
    std::unique_ptr<SpotLight> spotLight_ = nullptr;
};