#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <vector>
#include <list>
#include <string>
#include <memory>
#include <unordered_map>

#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"

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
    void Draw() override;
    
    std::string GetComponentName() const override { return "EnvironmentManagerComponent"; }
    void OnRegisterProperties() override;
    bool CanUpdateInEditMode() const override { return true; }

private:
    struct SpawnedEnvInfo {
        std::weak_ptr<GameObject> obj;
        std::string prefabPath;
        Irufemi::Vector3 originalPos;
        Irufemi::Vector3 originalRot;
        Irufemi::Vector3 originalScale;
    };

    struct BatchCollisionSetting {
        std::string prefabPath; // Now effectively 'prefabName' (e.g. Env_Pillar)
        Irufemi::Vector3 collisionSize;
        Irufemi::Vector3 previousSize;
        Irufemi::Vector3 collisionOffset;
        Irufemi::Vector3 previousOffset;
        int placementType; // 0: Building (スナップ), 1: Floating (そのまま)
        int previousPlacementType;
        bool isDestructible;
        int debrisSpawnCount;
    };

    std::string targetPrefabNames_ = "Env_Pillar,Env_Arch,Env_Wall";
    std::vector<SpawnedEnvInfo> spawnedObjects_;
    std::list<BatchCollisionSetting> batchCollisionSettings_;

    // モデルごとのバッチレンダラー
    std::unordered_map<std::string, std::unique_ptr<ModelBatchRendererComponent>> batchRenderers_;
};
