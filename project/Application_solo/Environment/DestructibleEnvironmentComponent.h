#pragma once
#include "Framework/Component/Component.h"
#include <string>

class DebrisManagerComponent;

/**
 * @class DestructibleEnvironmentComponent
 * @brief 破壊可能な環境物（ビルや壁など）にアタッチされるコンポーネント。
 * @details EnvironmentManagerComponentによって動的に付与され、瓦礫がヒットした際のダメージ判定と破壊イベント（瓦礫の飛散）を管理します。
 */
class DestructibleEnvironmentComponent : public Component {
public:
    DestructibleEnvironmentComponent() = default;
    ~DestructibleEnvironmentComponent() override = default;

    void Start() override;
    
    std::string GetComponentName() const override { return "DestructibleEnvironmentComponent"; }
    
    // パラメータ設定用
    void SetDebrisSpawnCount(int count) { debrisSpawnCount_ = count; }
    int GetDebrisSpawnCount() const { return debrisSpawnCount_; }

    // ダメージ処理
    void TakeDamage(int damage);

private:
    int hp_ = 1;
    int debrisSpawnCount_ = 3;
    
    DebrisManagerComponent* debrisManager_ = nullptr;
};
