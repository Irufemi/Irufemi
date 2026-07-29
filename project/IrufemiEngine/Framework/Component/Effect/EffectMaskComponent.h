#pragma once
#include "../Component.h"
#include <string>
#include "../../../Engine/Graphics/PostProcess/PostProcessManager.h"

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

    void SetEnableEffectMask(bool enable) { enableEffectMask_ = enable; }
    bool GetEnableEffectMask() const { return enableEffectMask_; }

    void SetCustomEffectType(int32_t type) { customEffectType_ = type; }
    int32_t GetCustomEffectType() const { return customEffectType_; }

    float GetCachedEffectParam() const { return cachedEffectParam_; }

    PostProcessManager::CustomEffectParams& GetCustomParams() { return customParams_; }
    const PostProcessManager::CustomEffectParams& GetCustomParams() const { return customParams_; }

private:
    void ApplyToRenderer();

    bool enableEffectMask_ = true;
    int32_t customEffectType_ = 0;
    float cachedEffectParam_ = 0.0f;
    PostProcessManager::CustomEffectParams customParams_;
    
    MeshRendererComponent* cachedRenderer_ = nullptr;
};
