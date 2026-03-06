#pragma once

#include <memory>
#include "Irufemi.h"

// 前方宣言
class Sprite;
class Camera;
class IrufemiEngine;

/**
 * @class Fade
 * @brief 画面全体のフェードイン・フェードアウト演出を管理するクラス
 */
class Fade {
public:
    // フェードの状態
    enum class FadeState {
        Idle,      // 待機中
        FadingIn,  // フェードイン中
        FadingOut, // フェードアウト中
    };

    Fade();
    ~Fade();

    /**
     * @brief 初期化処理
     * @param camera カメラのポインタ
     */
    void Initialize(Camera* camera);
    /**
     * @brief 更新処理
     */
    void Update();
    /**
     * @brief 描画処理
     */
    void Draw();

    /**
     * @brief フェードインを開始します
     * @param duration フェードにかかる時間(秒)
     * @param color フェードの色
     */
    void FadeIn(const float& duration, const Vector4& color);

    /**
     * @brief フェードアウトを開始します
     * @param duration フェードにかかる時間(秒)
     * @param color フェードの色
     */
    void FadeOut(const float& duration, const Vector4& color);

    /**
     * @brief フェードが完了したかどうか
     * @return true:完了 / false:実行中
     */
    bool IsDone() const { return state_ == FadeState::Idle; }

    static void SetEngine(IrufemiEngine* engine) { engine_ = engine; }

private:
    std::unique_ptr<Sprite> sprite_;
    FadeState state_ = FadeState::Idle;
    float fadeTimer_ = 0.0f;
    float fadeDuration_ = 0.0f;
    Vector4 fadeColor_ = { 0.0f, 0.0f, 0.0f, 1.0f };

    static IrufemiEngine* engine_;
};