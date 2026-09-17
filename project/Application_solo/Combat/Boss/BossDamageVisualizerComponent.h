#pragma once
#include "Framework/Component/Component.h"
#include <memory>

class BossComponent;
class GameObject;

/**
 * @class BossDamageVisualizerComponent
 * @brief ボスの被弾演出（カメラシェイク）および撃破演出を管理するコンポーネント
 */
class BossDamageVisualizerComponent : public Component {
public:
    BossDamageVisualizerComponent() = default;
    ~BossDamageVisualizerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void OnRegisterProperties() override;

    std::string GetComponentName() const override {
        return "BossDamageVisualizerComponent";
    }

    /**
     * @brief 被弾時のカメラシェイクを再生する
     * @param damage 受けたダメージ量
     */
    void TriggerDamageShake(float damage);

    /**
     * @brief 撃破時の特大カメラシェイクを再生する
     */
    void TriggerDeathShake();

    /**
     * @brief 撃破シェイクが現在再生中かどうかを取得する
     * @return 再生中の場合は true
     */
    bool IsDeathShakePlaying() const;

    float GetDamageShakeIntensity() const {
        return damageShakeIntensity_;
    }
    void SetDamageShakeIntensity(float val) {
        damageShakeIntensity_ = val;
    }

    int GetDamageShakeFrames() const {
        return damageShakeFrames_;
    }
    void SetDamageShakeFrames(int val) {
        damageShakeFrames_ = val;
    }

    float GetDamageShakeFrequency() const {
        return damageShakeFrequency_;
    }
    void SetDamageShakeFrequency(float val) {
        damageShakeFrequency_ = val;
    }

    float GetDeathShakeIntensity() const {
        return deathShakeIntensity_;
    }
    void SetDeathShakeIntensity(float val) {
        deathShakeIntensity_ = val;
    }

    int GetDeathShakeFrames() const {
        return deathShakeFrames_;
    }
    void SetDeathShakeFrames(int val) {
        deathShakeFrames_ = val;
    }

    float GetDeathShakeFrequency() const {
        return deathShakeFrequency_;
    }
    void SetDeathShakeFrequency(float val) {
        deathShakeFrequency_ = val;
    }

private:
    /**
     * @brief メインカメラのゲームオブジェクトを取得する（弱参照キャッシュ付き）
     */
    std::shared_ptr<GameObject> GetMainCamera();

private:
    BossComponent* bossComp_ = nullptr;
    std::weak_ptr<GameObject> mainCameraObj_;

    // 被弾シェイク
    float damageShakeIntensity_ = 0.4f;
    int damageShakeFrames_ = 10;
    float damageShakeFrequency_ = 15.0f;

    // 撃破シェイク
    float deathShakeIntensity_ = 2.0f;
    int deathShakeFrames_ = 60;
    float deathShakeFrequency_ = 10.0f;
};
