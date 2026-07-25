#pragma once
#include "../Component.h"
#include <string>

class MeshRendererComponent;

/**
 * @class EffectMaskComponent
 * @brief オブジェクト固有のポストプロセスエフェクト（アニメ塗り、グリッチ等）を指定するためのコンポーネント
 * @details このコンポーネントをアタッチすると、同じGameObjectにあるRendererのカスタムエフェクト値を上書きします。
 */
class EffectMaskComponent : public Component {
public:
    EffectMaskComponent();
    ~EffectMaskComponent() override;

    void Initialize() override;
    void Update() override;

    bool CanUpdateInEditMode() const override { return true; }

    std::string GetComponentName() const override { return "EffectMaskComponent"; }
    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    void SetEnableEffectMask(bool enable) { enableEffectMask_ = enable; ApplyToRenderer(); }
    bool GetEnableEffectMask() const { return enableEffectMask_; }

    void SetCustomEffectType(int32_t type) { customEffectType_ = type; ApplyToRenderer(); }
    int32_t GetCustomEffectType() const { return customEffectType_; }

    void SetCustomEffectParam(float param) { customEffectParam_ = param; ApplyToRenderer(); }
    float GetCustomEffectParam() const { return customEffectParam_; }

private:
    void ApplyToRenderer();

    bool enableEffectMask_ = true;
    int32_t customEffectType_ = 0;
    float customEffectParam_ = 0.0f;
    
    MeshRendererComponent* cachedRenderer_ = nullptr;
};
