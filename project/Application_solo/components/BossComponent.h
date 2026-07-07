#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Matrix4x4.h"
#include <vector>
#include <memory>
#include <string>

class DebrisManagerComponent;
class GameObject;

enum class BossState {
    Idle,           ///< 通常状態（シールドを纏っている）
    CoreExposed,    ///< シールドが全て剥がれ、コアが露出した状態
    Destroyed       ///< 撃破状態
};

class BossComponent : public Component {
public:
    BossComponent();
    ~BossComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;

    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "BossComponent"; }

    /**
     * @brief シールド（ガレキ）を1つ剥がし、本物のガレキとしてスポーンさせる
     * @return 剥がして生成されたガレキオブジェクト（失敗時はnullptr）
     */
    std::shared_ptr<GameObject> ExtractDebris();

    /**
     * @brief 特定のシールドをリストから除外する（破壊時など）
     */
    void RemoveShield(std::shared_ptr<GameObject> shield);

    /**
     * @brief ボスにダメージを与える（シールドがない場合のみ通る）
     */
    void TakeDamage(float damage);

    BossState GetState() const { return state_; }
    float GetHp() const { return hp_; }
    float GetMaxHp() const { return maxHp_; }

private:
    float maxHp_ = 1000.0f;
    float hp_ = 0.0f;

    int maxShieldCount_ = 100;
    
    float shieldRadius_ = 8.0f;

    BossState state_ = BossState::Idle;

    std::vector<std::shared_ptr<GameObject>> shields_;
    DebrisManagerComponent* debrisManager_ = nullptr;
    
    bool isShieldsInitialized_ = false;

    // --- ビーム攻撃用 ---
    class EnemyBeamComponent* beamComponent_ = nullptr;
    float beamTimer_ = 0.0f;
    float beamInterval_ = 10.0f; // 10秒おきに発射
    float beamOffsetZ_ = 25.0f;  // 発射位置のZ前方オフセット
    float beamOffsetY_ = -2.0f;  // 発射位置のYオフセット
    
    /** @brief ボスのビーム攻撃の最大射程距離 */
    float beamRange_ = 1000.0f;
};
