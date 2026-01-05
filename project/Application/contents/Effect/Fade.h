#pragma once

#include <memory>
#include "math/Vector4.h"
#include "function/Ease.h"

// 前方宣言
class Sprite;
class Camera;
class IrufemiEngine;

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

    // 初期化
    void Initialize(Camera* camera);
    // 更新
    void Update();
    // 描画
    void Draw();

    /// @brief フェードインを開始します
    /// @param duration フェードにかかる時間（秒）
    /// @param color フェードの色
    void FadeIn(float duration, const Vector4& color);

    /// @brief フェードアウトを開始します
    /// @param duration フェードにかかる時間（秒）
    /// @param color フェードの色
    void FadeOut(float duration, const Vector4& color);

    /// @brief フェードが完了したかどうか
    /// @return true:完了 / false:実行中
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