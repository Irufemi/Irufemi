#pragma once
#include "Framework/Component/Component.h"
#include <string>

/**
 * @class SpawnPointComponent
 * @brief 敵やオブジェクトの生成位置（起点）を示すコンポーネント。
 */
class SpawnPointComponent : public Component {
public:
    SpawnPointComponent() = default;
    ~SpawnPointComponent() override = default;

    void OnRegisterProperties() override;

    std::string GetComponentName() const override {
        return "SpawnPointComponent";
    }
    void Initialize() override;

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& json) override;

    // Getter / Setter
    const std::string& GetWaveId() const {
        return waveId_;
    }
    void SetWaveId(const std::string& id) {
        waveId_ = id;
    }

    const std::string& GetEnemyType() const {
        return enemyType_;
    }
    void SetEnemyType(const std::string& type) {
        enemyType_ = type;
    }

private:
    std::string waveId_ = "Wave1";
    std::string enemyType_ = "DefaultEnemy";
};
