#pragma once

#include "Framework/BaseScene.h"
#include "Framework/PromptController.h"
#include "Framework/UIAnimator.h"

#include <memory>
#include <vector>

class IrufemiEngine;
class Sprite;
class ObjClass;

/**
 * @class TitleScene
 * @brief タイトル画面を管理するクラス
 *
 * ゲームの開始をユーザーに促し、入力に応じてステージ選択シーンへ遷移します。
 * タイトルロゴのアニメーションやBGMの再生も担当します。
 */
class TitleScene : public BaseScene {
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
    PromptController promptController_;
    UIAnimator titleTextAnimator_;
};