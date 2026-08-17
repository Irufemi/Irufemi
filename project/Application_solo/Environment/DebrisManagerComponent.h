#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <memory>
#include <vector>
#include <queue>
#include <string>

class GameObject;
class VirtualEntityManagerComponent;

// がれきのアニメーション用並行データ（Data-Oriented Parallel Array）
struct DebrisAnimData {
    float baseIdleY_;
    float idleTimeY_;
};


/**
 * @class DebrisManagerComponent
 * @brief ガレキの生成・プール管理・検索を行うコンポーネント
 */
class DebrisManagerComponent : public Component {
public:
    DebrisManagerComponent() = default;
    ~DebrisManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "DebrisManagerComponent"; }

    /**
     * @brief プールからガレキを1つ取り出す
     */
    std::shared_ptr<GameObject> GetDebris();

    /**
     * @brief ガレキをプールに返却する（即時）
     */
    void ReleaseDebris(std::shared_ptr<GameObject> debris);

    /**
     * @brief ガレキをプール返却キューに積む（Update中の安全な削除用）
     */
    void MarkForRelease(std::shared_ptr<GameObject> debris);

    // プレイヤーからの引き寄せ処理用：指定座標から一番近い未昇格のがれきを実体化して返す
    std::shared_ptr<GameObject> ExtractNearestIdleDebris(const Irufemi::Vector3& pos, float radius);

    // 破壊通知
    void NotifyDestroyed(int virtualId, int variationIndex);

    // Debris パラメータのゲッター
    float GetDebrisPullSpeed() const { return debrisPullSpeed_; }
    float GetDebrisThrowSpeed() const { return debrisThrowSpeed_; }
    float GetDebrisOrbitSpeed() const { return debrisOrbitSpeed_; }
    float GetDebrisOrbitRadius() const { return debrisOrbitRadius_; }
    float GetDebrisDamage() const { return debrisDamage_; }
    float GetDebrisEnemyDamage() const { return debrisEnemyDamage_; }
    float GetDebrisPullYOffset() const { return debrisPullYOffset_; }
    float GetCameraShakeIntensity() const { return cameraShakeIntensity_; }
    int GetCameraShakeDurationFrames() const { return cameraShakeDurationFrames_; }
    Irufemi::Vector4 GetPlayerAuraColor() const { return playerAuraColor_; }
    Irufemi::Vector4 GetBossAuraColor() const { return bossAuraColor_; }
    Irufemi::Vector4 GetIdleAuraColor() const { return idleAuraColor_; }
    float GetCatchDistanceSq() const { return catchDistanceSq_; }
    float GetBossShieldRadius() const { return bossShieldRadius_; }

    Irufemi::Vector3 GetDebrisBaseScale() const { return debrisBaseScale_; }
    float GetColliderRadius() const { return colliderRadius_; }
    Irufemi::Vector3 GetAuraScale() const { return auraScale_; }
    float GetMaxThrowDistanceSq() const { return maxThrowDistance_ * maxThrowDistance_; }

private:
    // --- Data-Driven Variations ---
    struct DebrisVariation {
        std::string id;
        std::string modelPath;
        int maxPoolSize;
        int spawnWeight;
        VirtualEntityManagerComponent* virtualManager = nullptr;
        std::vector<DebrisAnimData> animDataList;
        std::queue<int> activeIds;
        std::shared_ptr<GameObject> poolObject;
    };
    std::vector<DebrisVariation> variations_;

    // --- Debris Settings ---
    float debrisPullSpeed_ = 10.0f;
    float debrisThrowSpeed_ = 50.0f;
    float debrisOrbitSpeed_ = 2.0f;
    float debrisOrbitRadius_ = 2.0f;
    float debrisDamage_ = 10.0f;
    float debrisEnemyDamage_ = 100.0f;
    float debrisPullYOffset_ = 2.0f;
    float cameraShakeIntensity_ = 0.5f;
    int cameraShakeDurationFrames_ = 10;
    Irufemi::Vector4 playerAuraColor_ = { 0.0f, 0.8f, 1.0f, 0.4f };
    Irufemi::Vector4 bossAuraColor_ = { 0.8f, 0.0f, 0.6f, 0.4f };
    Irufemi::Vector4 idleAuraColor_ = { 0.6f, 0.2f, 1.0f, 0.4f };
    float catchDistanceSq_ = 2.0f;
    float bossShieldRadius_ = 8.0f;

    Irufemi::Vector3 debrisBaseScale_ = { 0.5f, 0.5f, 0.5f };
    float colliderRadius_ = 0.5f;
    Irufemi::Vector3 auraScale_ = { 2.2f, 2.2f, 2.2f };
    float maxThrowDistance_ = 1500.0f;

    std::vector<std::shared_ptr<GameObject>> pendingReleases_;
};
