#pragma once

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "2D/Sprite.h"
#include "camera/Camera.h"
#include "actor/enemy/Enemy.h"
#include <memory>

/// <summary>
/// 敵 HP ゲージ UI
/// </summary>
class EnemyHpGauge {
public:
    EnemyHpGauge() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera">描画に使うカメラ（GameScene の camera_ を渡す）</param>
    /// <param name="enemy">対象の敵</param>
    void Initialize(Camera* camera, Enemy* enemy);

    /// <summary>
    /// 更新
    /// </summary>
    /// <param name="deltaTime">経過時間(秒)</param>
    void Update(float deltaTime);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

private:
    Camera* camera_ = nullptr;
    Enemy* enemy_ = nullptr;

    // 枠と中身のスプライト
    Sprite frameSprite_;
    Sprite fillSprite_;

    // 画面上の基準位置（画面サイズから決める）
    Vector2 basePos_{ 0.0f, 0.0f };

    // ゲージ全体のサイズ
    float barWidth_ = 800.0f;
    float barHeight_ = 50.0f;

    // 枠内の余白
    float paddingX_ = 4.0f;
    float paddingY_ = 4.0f;

    // 表示用 HP 割合（0〜1）
    float displayedRatio_ = 1.0f;

    // フェーズ遷移検出
    EnemyPhase prevPhase_ = EnemyPhase::Phase1;

    // ダメージ検出用
    int lastHp_ = -1;

    // フェーズ2移行時の回復アニメーション
    bool  healPlaying_ = false;
    float healTimer_ = 0.0f;
    float healDuration_ = 3.0f;     // 何秒かけて全回復しているように見せるか
    float healStartRatio_ = 1.0f;
    float healEndRatio_ = 1.0f;

    // HP ゲージのシェイク（幅だけ揺らす）
    bool  shakePlaying_ = false;
    float shakeTimer_ = 0.0f;
    float shakeDuration_ = 0.25f;    // 揺らす時間(秒)
    float shakeMagnitude_ = 0.15f;   // 幅に対する最大±割合（15%）
    float shakeFrequency_ = 40.0f;   // 揺れの速さ

    // 現在の幅スケール（1.0±α）
    float shakeScale_ = 1.0f;

private:
    float CalcHpRatio() const;
    void UpdateHeal(float deltaTime);
    void UpdateShake(float deltaTime);
    void UpdateSpriteTransform();

    // フェーズ1→2 への突入を検知した瞬間に呼ぶ
    void StartPhase2Heal();

    // HP が減った瞬間にシェイク開始
    void StartShakeOnDamage();

    static float EaseOutCubic(float t);
};
