#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/MathFunction.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Core/Utility/ObjectPool.h"

class GameObject;

/**
 * @brief 演出（エフェクト）のPrefabを一元管理し、再生（Instantiate または Poolからの取得）を行うマネージャー。
 *        シーン内に1つだけ配置されることを想定。
 */
class EffectManagerComponent : public Component {
public:
    EffectManagerComponent() = default;
    ~EffectManagerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "EffectManagerComponent"; }

    /**
     * @brief 指定したキーのエフェクトを指定したワールド座標で再生する
     * @param effectKey "Hit" などのエフェクトの種類を示すキー
     * @param worldPosition 再生する座標
     */
    void PlayEffect(const std::string& effectKey, const Irufemi::Vector3& worldPosition);

    /**
     * @brief シングルトン的なアクセスを提供する（シーン内に1つだけ存在する想定）
     */
    static EffectManagerComponent* GetInstance() { return instance_; }

private:
    static EffectManagerComponent* instance_;

    // エディタから設定する、代表的なエフェクトのPrefabパス
    std::string hitEffectPath_ = "resources/prefabs/normal_attack_hit_effect.json";
    std::string dustEffectPath_ = "resources/prefabs/debris_dust_effect.json";

    // 内部的にキーからパスを引くための辞書
    std::unordered_map<std::string, std::string> effectDictionary_;

    int maxHitEffects_ = 50;
    int maxDustEffects_ = 50;
    float effectDuration_ = 2.0f; // エフェクトの生存時間
    std::unique_ptr<ObjectPool<GameObject>> hitEffectPool_;
    std::unique_ptr<ObjectPool<GameObject>> dustEffectPool_;

    struct ActiveEffect {
        ObjectPool<GameObject>::Handle handle;
        float timer;
        std::string effectKey;
    };
    std::vector<ActiveEffect> activeEffects_;
};
