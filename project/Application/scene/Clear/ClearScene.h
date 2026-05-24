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
 * @class ResultScene
 * @brief ゲームの結果（クリア/ゲームオーバー）を表示するクラス
 *
 * ゲームの結果に応じてUIを表示し、入力に応じてステージ選択シーンへ戻ります。
 */
class ClearScene : public BaseScene {
public: // メンバ関数(システム)
    ~ClearScene() override;
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

    // 「Clear!!」文字
    std::unique_ptr<ObjClass> clearTextC_ = nullptr;
    std::unique_ptr<ObjClass> clearTextL_ = nullptr;
    std::unique_ptr<ObjClass> clearTextE_ = nullptr;
    std::unique_ptr<ObjClass> clearTextA_ = nullptr;
    std::unique_ptr<ObjClass> clearTextR_ = nullptr;
    std::unique_ptr<ObjClass> clearTextEx_ = nullptr;

    // 「Push to Space」文字
    std::unique_ptr<ObjClass> textPushToSpace_ = nullptr;

    // 祝祭パーティクル
    std::unique_ptr<GPUParticleSystem> confettiParticles_ = nullptr;

    // 演出状態管理
    float introTimer_ = 0.0f;
    int currentSlamIndex_ = -1; // 落とした文字の数
    bool isSlamming_ = true;
    bool isRainingConfetti_ = false; // 降らせる演出に移行したか

private: // メンバ変数(システム)

    PromptController promptController_;
    UIAnimator clearTextAnimator_;
};
