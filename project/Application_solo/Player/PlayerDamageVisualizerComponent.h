#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector4.h"
#include <memory>

class PlayerHealthComponent;
class BaseModel;

/**
 * @class PlayerDamageVisualizerComponent
 * @brief プレイヤーの被弾演出（モデル点滅、カメラシェイク、画面エフェクト）および死亡演出を担うコンポーネント
 */
class PlayerDamageVisualizerComponent : public Component {
public:
    PlayerDamageVisualizerComponent() = default;
    ~PlayerDamageVisualizerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;

    std::string GetComponentName() const override {
        return "PlayerDamageVisualizerComponent";
    }

    /**
     * @brief 被弾時の点滅演出を開始する
     */
    void TriggerDamageFlash();

    /**
     * @brief カメラシェイク演出を再生する
     */
    void TriggerCameraShake();

    /**
     * @brief 画面エフェクト（ポストエフェクト）を再生する
     */
    void TriggerScreenEffect();

    /**
     * @brief 死亡時のモデル非表示演出を実行する
     */
    void TriggerDeathVisuals();

    float GetFlashInterval() const {
        return flashInterval_;
    }
    void SetFlashInterval(float interval) {
        flashInterval_ = interval;
    }

    float GetFlashDuration() const {
        return flashDuration_;
    }
    void SetFlashDuration(float duration) {
        flashDuration_ = duration;
    }

    const Irufemi::Vector4& GetFlashColor() const {
        return flashColor_;
    }
    void SetFlashColor(const Irufemi::Vector4& color) {
        flashColor_ = color;
    }

private:
    /**
     * @brief 対象のBaseModelポインタを安全に取得する
     * @return 取得できたBaseModelへのポインタ、存在しない場合はnullptr
     */
    BaseModel* GetTargetModel();

private:
    PlayerHealthComponent* healthComp_ = nullptr; ///< 監視対象の体力コンポーネント

    // 点滅演出パラメータ
    float flashTimer_ = 0.0f;
    float flashDuration_ = 1.0f;
    float flashRemaining_ = 0.0f;
    float flashInterval_ = 0.05f;
    bool isFlashing_ = false;

    Irufemi::Vector4 flashColor_ = {1.0f, 0.0f, 0.0f, 1.0f};        ///< 点滅カラー
    Irufemi::Vector4 originalBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f}; ///< 元のモデルカラー
    bool colorCached_ = false;

    // カメラシェイクパラメータ
    float shakeIntensity_ = 1.0f;
    int shakeFrames_ = 30;
    float shakeFrequency_ = 20.0f;
};
