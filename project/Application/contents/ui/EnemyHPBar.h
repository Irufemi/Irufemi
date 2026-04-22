#pragma once

#include <memory>

class Sprite;
class Camera;
class Enemy;

/**
 * @class EnemyHPBar
 * @brief 画面上部に敵のHPバーを表示するUIクラス（ImGui不使用・Release対応）
 */
class EnemyHPBar {
public:
    ~EnemyHPBar() = default;

    /**
     * @brief 初期化
     * @param camera 2D描画に使用するカメラ
     * @param screenWidth 画面横幅
     * @param screenHeight 画面縦幅
     */
    void Initialize(Camera* camera, int screenWidth, int screenHeight);

    /**
     * @brief 毎フレーム更新（HP割合に基づいてバーの幅と色を更新）
     * @param enemy 敵のポインタ
     */
    void Update(const Enemy* enemy);

    /**
     * @brief スプライト描画（ApplySpritePSO の後に呼ぶこと）
     */
    void Draw();

private:
    /// HP割合からバーの色を計算する（緑→黄→赤のグラデーション）
    void UpdateBarColor(float hpRatio);

    // バー表示用スプライト
    std::unique_ptr<Sprite> barBg_;       ///< 背景（暗いバー）
    std::unique_ptr<Sprite> barFill_;     ///< HP残量（色付きバー）
    std::unique_ptr<Sprite> barFrame_;    ///< 枠線

    // レイアウト定数
    float barX_ = 0.0f;       ///< バー左端のX座標
    float barY_ = 0.0f;       ///< バー上端のY座標
    float barMaxWidth_ = 0.0f; ///< バーの最大幅
    float barHeight_ = 0.0f;   ///< バーの高さ

    // 現在のHP割合（0.0〜1.0）
    float currentRatio_ = 1.0f;

    // アニメーション用（ダメージ時のスムーズな減少）
    float displayRatio_ = 1.0f;

    // 最大HP（Initialize後に固定）
    int maxHP_ = 1;
};
