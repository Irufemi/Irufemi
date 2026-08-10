#pragma once
#include "WaveEvent.h"

// 敵スポーン用のハンドラ
class SpawnEnemyHandler : public IWaveEventHandler {
public:
    void Execute(const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) override;
};

// BGM変更用のハンドラ（ダミー実装）
class PlayBGMHandler : public IWaveEventHandler {
public:
    void Execute(const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) override;
};
