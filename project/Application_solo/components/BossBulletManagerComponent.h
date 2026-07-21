#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <memory>
#include <vector>
#include <queue>

class GameObject;
class VirtualEntityManagerComponent;

class BossBulletManagerComponent : public Component {
public:
    BossBulletManagerComponent();
    ~BossBulletManagerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;

    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "BossBulletManagerComponent"; }

    /**
     * @brief 指定した座標と速度で弾を発射する
     */
    void SpawnBullet(const Vector3& position, const Vector3& velocity);

    /**
     * @brief 弾の回収（寿命や被弾など）
     */
    void ReleaseBullet(int virtualId);

private:
    struct BossBulletData {
        Vector3 velocity;
        float lifeTimer;
    };

    int maxBullets_ = 2000;
    float defaultLifeTime_ = 5.0f;
    Vector3 bulletScale_ = { 0.5f, 0.5f, 0.5f };

    VirtualEntityManagerComponent* virtualManager_ = nullptr;
    std::vector<BossBulletData> bulletDataList_;
    std::queue<int> activeVirtualIds_;
};
