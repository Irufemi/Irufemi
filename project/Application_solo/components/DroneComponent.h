#pragma once
#include "Framework/Component/Component.h"
#include <memory>

class GameObject;
class BossComponent;
class BossBulletManagerComponent;

class DroneComponent : public Component {
public:
    DroneComponent();
    ~DroneComponent() override = default;

    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "DroneComponent"; }

    void SetOrbit(std::weak_ptr<GameObject> boss, float radius, float initialAngle, float speed);
    void TakeDamage(float damage);

    void SetBulletManager(BossBulletManagerComponent* bulletMgr) { bulletManager_ = bulletMgr; }

private:
    void FireBullet();

    std::weak_ptr<GameObject> boss_;
    std::weak_ptr<GameObject> player_;
    float orbitRadius_ = 15.0f;
    float orbitAngle_ = 0.0f;
    float orbitSpeed_ = 1.0f;
    
    float hp_ = 1.0f; // ガレキ1発で壊れる想定

    float fireTimer_ = 0.0f;
    float fireInterval_ = 2.0f; // ランダムな間隔で発射

    BossBulletManagerComponent* bulletManager_ = nullptr;

    // デバッグ用
    bool hasPlayer_ = false;
    Vector3 debugPlayerPos_{};
};
