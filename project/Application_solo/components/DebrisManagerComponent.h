#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Utility/ObjectPool.h"
#include <memory>

class GameObject;

/**
 * @class DebrisManagerComponent
 * @brief ガレキの生成・プール管理・検索を行うコンポーネント
 */
class DebrisManagerComponent : public Component {
public:
    DebrisManagerComponent() = default;
    ~DebrisManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "DebrisManagerComponent"; }

    // ガレキの取得と返却
    std::shared_ptr<GameObject> AcquireDebris();
    void ReleaseDebris(std::shared_ptr<GameObject> debris);

private:
    std::unique_ptr<ObjectPool<GameObject>> pool_;
    int poolSize_ = 100;
    bool isPoolInitialized_ = false;
};
