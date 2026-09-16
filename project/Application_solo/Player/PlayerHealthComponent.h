#pragma once
#include "Framework/Component/Component.h"
#include <functional>
#include <string>
#include <vector>

/**
 * @class PlayerHealthComponent
 * @brief プレイヤーの体力、被弾ダメージ、無敵時間、死亡状態を管理するコンポーネント
 */
class PlayerHealthComponent : public Component {
public:
    PlayerHealthComponent() = default;
    ~PlayerHealthComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override {
        return "PlayerHealthComponent";
    }

    std::function<void()> onPlayerDied;
    std::function<void()> onDeathSequenceFinished;

    /**
     * @brief 被弾時のイベントリスナーを追加する
     */
    void AddOnDamageTakenListener(std::function<void(int damage)> callback) {
        onDamageTakenListeners_.push_back(std::move(callback));
    }

    /**
     * @brief 死亡時のイベントリスナーを追加する
     */
    void AddOnPlayerDiedListener(std::function<void()> callback) {
        onPlayerDiedListeners_.push_back(std::move(callback));
    }

    void LoadStatusFromJson();

    std::string GetStatusDataPath() const {
        return statusDataPath_;
    }
    void SetStatusDataPath(const std::string& path) {
        statusDataPath_ = path;
    }

    void TakeDamage(int damage);
    bool IsInvincible() const {
        return invincibilityTimer_ > 0.0f;
    }
    float GetInvincibilityTimer() const {
        return invincibilityTimer_;
    }
    float GetMaxInvincibilityTime() const {
        return maxInvincibilityTime_;
    }
    void SetGodMode(bool godMode) {
        isGodMode_ = godMode;
    }

    int GetHp() const {
        return hp_;
    }
    int GetMaxHp() const {
        return maxHp_;
    }
    bool IsDead() const {
        return isDead_;
    }
    bool IsGodMode() const {
        return isGodMode_;
    }

private:
    std::string statusDataPath_ = "resources/GameData/PlayerStatus.json";
    int hp_ = 100;
    int maxHp_ = 100;
    bool isDead_ = false;
    bool isGodMode_ = false;

    float deathStartTime_ = 0.0f;
    bool hasTriggeredDeathSequenceFinished_ = false;

    // 被弾・無敵時間
    float invincibilityTimer_ = 0.0f;
    float maxInvincibilityTime_ = 1.0f;

    std::vector<std::function<void(int damage)>> onDamageTakenListeners_;
    std::vector<std::function<void()>> onPlayerDiedListeners_;
};

