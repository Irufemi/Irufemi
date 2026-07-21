#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <vector>
#include <string>
#include <memory>

class GameObject;

/**
 * @class EnvironmentManagerComponent
 * @brief ゲーム内の環境オブジェクト（柱、壁、アーチなどの建造物）の動的配置を管理するコンポーネント。
 * @details JSONプレハブを読み込み、シーンの初期化時に指定された座標へ一括でスポーンさせる役割を持つ。
 * 将来的にはデータ駆動による配置や、インスタンシング描画への移行を想定している。
 */
class EnvironmentManagerComponent : public Component {
public:
    EnvironmentManagerComponent() = default;
    ~EnvironmentManagerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    
    std::string GetComponentName() const override { return "EnvironmentManagerComponent"; }

private:
    struct EnvSpawnData {
        std::string prefabPath;
        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
    };

    std::vector<EnvSpawnData> spawnList_;
    std::vector<std::weak_ptr<GameObject>> spawnedObjects_;

    void LoadSpawnList();
    void SpawnAll();
};
