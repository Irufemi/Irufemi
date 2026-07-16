#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"

class DebrisManagerComponent;

enum class DebrisState {
    Idle,       ///< 漂流中（自然な浮遊）
    Pulled,     ///< プレイヤーに引き寄せられている
    Orbiting,   ///< プレイヤーの周囲を回転浮遊中
    BossOrbiting, ///< ボスの周囲を回転浮遊中
    Thrown      ///< 敵へ向かってホーミング中
};

/**
 * @class DebrisComponent
 * @brief ガレキの振る舞い（状態遷移と位置の補間）を制御するコンポーネント
 */
class DebrisComponent : public Component {
public:
    DebrisComponent() = default;
    ~DebrisComponent() override = default;

    void Initialize() override;
    void OnEnable() override;
    void Update() override;
    void OnCollisionEnter(GameObject* hitObject) override;
    std::string GetComponentName() const override { return "DebrisComponent"; }

    // 状態変更用のインターフェース
    void SetState(DebrisState newState);
    DebrisState GetState() const { return state_; }

    void SetTarget(std::weak_ptr<GameObject> target) { targetObject_ = target; }
    std::weak_ptr<GameObject> GetTarget() const { return targetObject_; }
    void SetOrbitParams(float angle, float radius) { orbitAngle_ = angle; orbitRadius_ = radius; }
    void SetThrowDirection(const Vector3& dir) { throwDirection_ = dir; }

    void SetVirtualId(int id) { virtualId_ = id; }
    int GetVirtualId() const { return virtualId_; }
    void SetManager(DebrisManagerComponent* manager) { manager_ = manager; }

private:
    DebrisState state_ = DebrisState::Idle;
    
    int virtualId_ = -1;
    DebrisManagerComponent* manager_ = nullptr;
    
    // 追従・目標用の対象
    std::weak_ptr<GameObject> targetObject_;
    
    // パラメータ取得用ヘルパー（Managerから取得）
    float GetPullSpeed() const;
    float GetThrowSpeed() const;
    float GetOrbitSpeed() const;
    float GetBossDamage() const;
    float GetEnemyDamage() const;
    float GetCameraShakeIntensity() const;
    int GetCameraShakeDurationFrames() const;
    Vector4 GetPlayerAuraColor() const;
    Vector4 GetBossAuraColor() const;
    float GetCatchDistanceSq() const;
    float GetBossShieldRadius() const;
    float GetPullYOffset() const;

    // Orbit Radiusは動的に設定されるためローカルに保持
    float orbitRadius_ = 2.0f;

    // 内部状態
    float baseIdleY_ = 0.0f;
    float idleTimeY_ = 0.0f;
    float orbitAngle_ = 0.0f;
    Vector3 throwDirection_ = {0,0,0};
    Vector3 throwOrigin_ = {0,0,0};

    // ボス用Orbitパラメータ
    float bossOrbitAngleX_ = 0.0f;
    float bossOrbitAngleY_ = 0.0f;
    float bossOrbitAngleZ_ = 0.0f;
    float bossOrbitSpeedX_ = 0.0f;
    float bossOrbitSpeedY_ = 0.0f;
    float bossOrbitSpeedZ_ = 0.0f;
    float bossOrbitRadiusOffset_ = 0.0f;
};
