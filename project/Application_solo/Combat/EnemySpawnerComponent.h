#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
#include "Core/Utility/ObjectPool.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include <memory>
#include <unordered_map>

class EnemySpawnerComponent : public Component {
public:
    EnemySpawnerComponent() = default;
    ~EnemySpawnerComponent() override;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override {
        return "EnemySpawnerComponent";
    }

    /**
     * @brief 指定した座標・回転・スケール倍率でエネミーをスポーンする
     * @param[in] position スポーンワールド座標
     * @param[in] rotation スポーン回転角度
     * @param[in] scaleMultiplier スケール倍率
     * @return 生成またはプールから再利用された GameObject
     */
    GameObject* SpawnEnemy(const Irufemi::Vector3& position, const Irufemi::Vector3& rotation,
                           float scaleMultiplier = 1.0f);

    /**
     * @brief エネミーのモデルパスを取得する
     * @return モデルファイルパス
     */
    const std::string& GetEnemyModelPath() const {
        return enemyModelPath_;
    }
    /**
     * @brief エネミーのモデルパスを設定する
     * @param[in] path モデルファイルパス
     */
    void SetEnemyModelPath(const std::string& path) {
        enemyModelPath_ = path;
    }

    /**
     * @brief 基本エネミースケールを取得する
     * @return 基本スケール
     */
    const Irufemi::Vector3& GetBaseEnemyScale() const {
        return baseEnemyScale_;
    }
    /**
     * @brief 基本エネミースケールを設定する
     * @param[in] scale 基本スケール
     */
    void SetBaseEnemyScale(const Irufemi::Vector3& scale) {
        baseEnemyScale_ = scale;
    }

    /**
     * @brief 基本コライダー半径を取得する
     * @return 基本コライダー半径
     */
    float GetBaseColliderRadius() const {
        return baseColliderRadius_;
    }
    /**
     * @brief 基本コライダー半径を設定する
     * @param[in] radius 基本コライダー半径
     */
    void SetBaseColliderRadius(float radius) {
        baseColliderRadius_ = radius;
    }

private:
    int maxEnemies_ = 50;
    std::string enemyModelPath_ = "Enemy_GravityGolem_A/SM_Enemy_GravityGolem_A.obj";
    std::string enemyPrefabPath_ = "resources/prefabs/Enemy_GravityGolem.json";
    Irufemi::Vector3 baseEnemyScale_ = {1.2f, 1.2f, 1.2f};
    float baseColliderRadius_ = 2.0f;
    std::unique_ptr<ObjectPool<GameObject>> enemyPool_;
    std::unordered_map<GameObject*, ObjectPool<GameObject>::Handle> activeEnemyHandles_;
    std::shared_ptr<ModelBatchRendererComponent> batchRenderer_;
};
