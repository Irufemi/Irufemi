#pragma once

#include "Framework/IScene.h"

#include <memory>
#include <vector>

class IrufemiEngine;
class Camera;
class DebugCamera;
class Sprite;
class ObjClass;
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

    // 3Dタイトル文字（七転び八転び）
    std::unique_ptr<ObjClass> titleTextNana_ = nullptr;
    std::unique_ptr<ObjClass> titleTextKoro1_ = nullptr;
    std::unique_ptr<ObjClass> titleTextBi1_ = nullptr;
    std::unique_ptr<ObjClass> titleTextHati_ = nullptr;
    std::unique_ptr<ObjClass> titleTextKoro2_ = nullptr;
    std::unique_ptr<ObjClass> titleTextBi2_ = nullptr;

    // 「Push to Space」文字
    std::unique_ptr<ObjClass> titleTextPushToSpace_ = nullptr;

private: // メンバ変数(システム)
    // エンジン
    IrufemiEngine* engine_ = nullptr;
    // カメラ
    std::unique_ptr<Camera> camera_ = nullptr;
    // デバッグカメラ
    std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

    bool isChangingScene_ = false;
    bool isTransitionRequested_ = false; // 遷移処理をSceneManagerに渡したか
    float transitionDelayTimer_ = 0.0f;  // 決定から遷移開始までの遅延タイマー
    bool isDrawPushToSpace_ = true;      // 決定時のフラッシュで描画自体をスキップするためのフラグ

    bool debugMode_ = false;
    float animationTime_ = 0.0f; // アニメーション用タイマー
    // ライト
    std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;
    std::vector<std::unique_ptr<PointLight>> pointLights_;
    std::vector<std::unique_ptr<SpotLight>> spotLights_;
    std::vector<std::unique_ptr<AreaLight>> areaLights_;
};