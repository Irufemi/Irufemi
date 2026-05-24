#pragma once

#include "Framework/BaseScene.h"
#include "Framework/PromptController.h"
#include "Framework/UIAnimator.h"
#include "Renderer/Object2D/Sprite/Sprite.h"
#include <memory>
#include <vector>

class IrufemiEngine;
class ObjClass;

class GPUParticleSystem;

/**
 * @class GameOverScene
 * @brief ゲームの結果（クリア/ゲームオーバー）を表示するクラス
 *
 * ゲームの結果に応じてUIを表示し、入力に応じてステージ選択シーンへ戻ります。
 */
class GameOverScene : public BaseScene {
public: // メンバ関数(システム)
    ~GameOverScene() override;
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

private: // メンバ変数(ゲーム)

    // 「GameOver...」文字
    std::unique_ptr<ObjClass> goTextG_ = nullptr;
    std::unique_ptr<ObjClass> goTextA_ = nullptr;
    std::unique_ptr<ObjClass> goTextM_ = nullptr;
    std::unique_ptr<ObjClass> goTextE1_ = nullptr;
    std::unique_ptr<ObjClass> goTextO_ = nullptr;
    std::unique_ptr<ObjClass> goTextV_ = nullptr;
    std::unique_ptr<ObjClass> goTextE2_ = nullptr;
    std::unique_ptr<ObjClass> goTextR_ = nullptr;
    std::unique_ptr<ObjClass> goTextDot_ = nullptr;

    // 「Push to Space」文字
    std::unique_ptr<ObjClass> textPushToSpace_ = nullptr;

    // 灰パーティクル
    std::unique_ptr<GPUParticleSystem> embersParticles_ = nullptr;

    // 演出用
    float introTimer_ = 0.0f;
    float cameraZ_ = -10.0f;

private: // メンバ変数(システム)

    PromptController promptController_;
    UIAnimator goTextAnimator_;
};
