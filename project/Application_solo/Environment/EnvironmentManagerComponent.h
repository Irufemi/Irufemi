#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
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

    std::string GetComponentName() const override {
        return "EnvironmentManagerComponent";
    }
    void OnRegisterProperties() override;
    bool CanUpdateInEditMode() const override {
        return true;
    }

private:
    struct SpawnedEnvInfo {
        std::weak_ptr<GameObject> obj_;
        std::string prefabPath_;
        Irufemi::Vector3 originalPos_;
        Irufemi::Vector3 originalRot_;
        Irufemi::Vector3 originalScale_;
    };

    struct BatchCollisionSetting {
        std::string prefabPath_; // Now effectively 'prefabName' (e.g. Env_Pillar)
        Irufemi::Vector3 collisionSize_;
        Irufemi::Vector3 previousSize_;
        Irufemi::Vector3 collisionOffset_;
        Irufemi::Vector3 previousOffset_;
        int placementType_ = 0; // 0: Building (スナップ), 1: Floating (そのまま)
        int previousPlacementType_ = 0;
        bool isDestructible_ = false;
        int debrisSpawnCount_ = 3;
        Irufemi::Vector3 pushbackMask_ = {1.0f, 1.0f, 1.0f};
        Irufemi::Vector3 previousPushbackMask_ = {1.0f, 1.0f, 1.0f};
    };

    std::string targetPrefabNames_ = "Env_Pillar,Env_Arch,Env_Wall";
    std::vector<SpawnedEnvInfo> spawnedObjects_;
    std::list<BatchCollisionSetting> batchCollisionSettings_;

    // モデルごとのバッチレンダラー
    std::unordered_map<std::string, std::unique_ptr<ModelBatchRendererComponent>> batchRenderers_;

    /**
     * @brief 指定モデルのバッチレンダラーを取得（未生成なら生成して初期化・キャッシュ）
     * @param modelName 3Dモデル名
     * @return バッチレンダラーへのポインタ（生成失敗時は nullptr）
     */
    ModelBatchRendererComponent* GetOrCreateBatchRenderer(const std::string& modelName);
};
