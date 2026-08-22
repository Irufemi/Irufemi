#pragma once
#include "Framework/Component/Component.h"
#include <string>

class GameLoopManagerComponent : public Component {
public:
    enum class State {
        Playing,
        Finished
    };

    GameLoopManagerComponent() = default;
    ~GameLoopManagerComponent() override = default;

    void Initialize() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "GameLoopManagerComponent"; }

private:
    State state_ = State::Playing;
    float resultDelayTime_ = 3.0f;
    float timeScaleAtResult_ = 0.1f;
    float timer_ = 0.0f;
    bool isClear_ = false;

    std::string targetPlayerName_ = "Player";
    std::string targetBossName_ = "Boss";

    class GravityPlayerComponent* player_ = nullptr;
    class BossComponent* boss_ = nullptr;
};
