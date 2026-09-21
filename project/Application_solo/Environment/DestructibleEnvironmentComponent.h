#pragma once
#include "Framework/Component/Component.h"
#include "Combat/IDamageable.h"
#include <string>

class DebrisManagerComponent;
class EffectManagerComponent;

/**
 * @class DestructibleEnvironmentComponent
 * @brief 破壊可能な環境物（ビルや壁など）にアタッチされるコンポーネント。
 * @details
 * EnvironmentManagerComponentによって動的に付与され、瓦礫がヒットした際のダメージ判定と破壊イベント（瓦礫の飛散）を管理します。
 */
class DestructibleEnvironmentComponent : public Component, public IDamageable {
public:
    DestructibleEnvironmentComponent() = default;
    ~DestructibleEnvironmentComponent() override = default;

    void Initialize() override;
    void Start() override;

    std::string GetComponentName() const override {
        return "DestructibleEnvironmentComponent";
    }

    // パラメータ設定用
    void SetDebrisSpawnCount(int count) {
        debrisSpawnCount_ = count;
    }
    int GetDebrisSpawnCount() const {
        return debrisSpawnCount_;
    }

    // ダメージ処理
    void TakeDamage(float damage) override {
        TakeDamage(static_cast<int>(damage));
    }
    void TakeDamage(int damage);

    DamageableType GetDamageableType() const override {
        return DamageableType::Environment;
    }

private:
    DebrisManagerComponent* GetDebrisManager();
    EffectManagerComponent* GetEffectManager();

    int hp_ = 1;
    int debrisSpawnCount_ = 3;

    DebrisManagerComponent* debrisManager_ = nullptr; ///< 破片生成マネージャーへの参照
    /**
     * @brief 破壊エフェクトを再生するためのエフェクトマネージャーへの参照
     */
    EffectManagerComponent* effectManager_ = nullptr;
};
