#pragma once
#include "Framework/Component/Component.h"
#include "Level/WaveEvent.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>

class SplineFollowerComponent;
class SpawnPointComponent;

/**
 * @class WaveManagerComponent
 * @brief 外部JSONデータに基づき、レール上の進行距離に応じてイベントをディスパッチする
 */
class WaveManagerComponent : public Component {
public:
    WaveManagerComponent();
    ~WaveManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    bool CanUpdateInEditMode() const override { return true; }
    
    std::string GetComponentName() const override { return "WaveManagerComponent"; }
    void OnRegisterProperties() override;
    
    void ReloadLevelData();

    /**
     * @brief 特定のイベント種類に対するハンドラを登録する
     */
    void RegisterHandler(const std::string& eventType, std::shared_ptr<IWaveEventHandler> handler);

    /**
     * @brief シーン内の SpawnPointComponent を取得してキャッシュする
     */
    void CacheSpawnPoints();
    const std::vector<SpawnPointComponent*>& GetSpawnPoints(const std::string& waveId) const;

private:
    void LoadLevelData(const std::string& filePath);

    std::priority_queue<WaveEventData, std::vector<WaveEventData>, std::greater<WaveEventData>> eventQueue_;
    std::vector<WaveEventData> allEvents_; // パース済みの全イベントリスト（エディタのプレビューおよびUI用）
    std::unordered_map<std::string, std::shared_ptr<IWaveEventHandler>> handlers_;
    
    // キャッシュ
    std::unordered_map<std::string, std::vector<SpawnPointComponent*>> spawnPointsMap_;
    bool hasCachedSpawnPoints_ = false;
    
    SplineFollowerComponent* playerFollower_ = nullptr;
    std::string levelDataPath_ = "resources/GameData/WaveData_Stage1.json";

public:
    const std::vector<WaveEventData>& GetAllEvents() const { return allEvents_; }
    const std::string& GetLevelDataPath() const { return levelDataPath_; }
    void SetLevelDataPath(const std::string& path) { levelDataPath_ = path; }
};
