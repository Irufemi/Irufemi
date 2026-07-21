#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Utility/ObjectPool.h"
#include <memory>
#include <unordered_map>
#include <vector>

class GameObject;
class BossBulletManagerComponent;

class DroneManagerComponent : public Component {
public:
    DroneManagerComponent();
    ~DroneManagerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;

    std::string GetComponentName() const override { return "DroneManagerComponent"; }
    void OnRegisterProperties() override;

    /**
     * @brief ボスの周囲にドローンを一斉展開する
     */
    void DeployDrones(std::weak_ptr<GameObject> boss, int count, BossBulletManagerComponent* bulletMgr);

    /**
     * @brief ドローンを全回収する
     */
    void RecallAllDrones();

private:
    int maxDrones_ = 50;
    std::unique_ptr<ObjectPool<GameObject>> dronePool_;
    std::vector<GameObject*> activeDrones_;
};
