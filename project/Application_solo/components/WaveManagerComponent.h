#pragma once
#include "Framework/Component/Component.h"
#include "WaveEvent.h"
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>

class SplineFollowerComponent;

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
    
    std::string GetComponentName() const override { return "WaveManagerComponent"; }
    void OnRegisterProperties() override;

    /**
     * @brief 特定のイベント種類に対するハンドラを登録する
     */
    void RegisterHandler(const std::string& eventType, std::shared_ptr<IWaveEventHandler> handler);

private:
    void LoadLevelData(const std::string& filePath);

    std::priority_queue<WaveEventData, std::vector<WaveEventData>, std::greater<WaveEventData>> eventQueue_;
    std::unordered_map<std::string, std::shared_ptr<IWaveEventHandler>> handlers_;
    
    SplineFollowerComponent* playerFollower_ = nullptr;
    std::string levelDataPath_ = "resources/configs/WaveData_Stage1.json";
};
