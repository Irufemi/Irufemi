#pragma once

#include <memory>
#include <vector>
#include "2D/Sprite.h"

class Camera;
class IrufemiEngine;

/// <summary>
/// スマブラ風ストライプトランジション
/// 入り（画面を覆う）とはけ（画面から消える）の両方に対応
/// </summary>
class StripeTransition {
public:
    /// <summary>
    /// トランジションの種類
    /// </summary>
    enum class Mode {
        In,   // 画面を覆う（入り）
        Out,  // 画面から消える（はけ）
    };

    StripeTransition() = default;
    ~StripeTransition() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera">カメラ</param>
    /// <param name="engine">エンジン</param>
    /// <param name="mode">In=覆う, Out=はける</param>
    void Initialize(Camera* camera, IrufemiEngine* engine, Mode mode);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// トランジション開始
    /// </summary>
    void Start();

    /// <summary>
    /// トランジションが完了したか
    /// </summary>
    bool IsFinished() const { return isFinished_; }

    /// <summary>
    /// トランジション中か
    /// </summary>
    bool IsActive() const { return isActive_; }

    // パラメータ設定
    void SetStripeCount(int count) { stripeCount_ = count; }
    void SetStripeSize(float width, float height) { stripeWidth_ = width; stripeHeight_ = height; }
    void SetMoveSpeed(float speed) { moveSpeed_ = speed; }
    void SetSpacingOffset(float offset) { spacingOffset_ = offset; }

private:
    // スプライト
    std::vector<std::unique_ptr<Sprite>> stripes_;
    std::vector<float> progress_;

    // エンジン参照
    IrufemiEngine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    // モード
    Mode mode_ = Mode::In;

    // 状態
    bool isActive_ = false;
    bool isFinished_ = false;

    // パラメータ
    int stripeCount_ = 8;
    float stripeWidth_ = 700.0f;
    float stripeHeight_ = 820.0f;
    float moveSpeed_ = 0.1f;
    float spacingOffset_ = 50.0f;

    // テクスチャパス
    std::string texturePath_ = "resources/texture/stripe.png";
};
