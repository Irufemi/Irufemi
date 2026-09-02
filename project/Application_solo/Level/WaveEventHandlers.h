#pragma once
#include "Level/WaveEvent.h"

// 敵スポーン用のハンドラ
class SpawnEnemyHandler : public IWaveEventHandler {
public:
    void Execute(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos,
                 const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) override;

#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
    void DrawEditorPreview(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos,
                           const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) override;
#endif

private:
    std::vector<Irufemi::Vector3> CalculateSpawnPositions(WaveManagerComponent* manager, const WaveEventData& data,
                                                          const Irufemi::Vector3& railPos,
                                                          const Irufemi::Vector3& railForward,
                                                          const Irufemi::Vector3& railRight);
};

// BGM変更用のハンドラ（ダミー実装）
class PlayBGMHandler : public IWaveEventHandler {
public:
    void Execute(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos,
                 const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) override;
};
