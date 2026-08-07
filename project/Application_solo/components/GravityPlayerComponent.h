#pragma once
#include "Framework/Component/Component.h"
#include <vector>
#include <memory>

class GameObject;
class PlayerTargetingComponent;

/**
 * @class GravityPlayerComponent
 * @brief ガレキを引き寄せて投げる「重力スロー」アクションを制御するコンポーネント
 */
class GravityPlayerComponent : public Component {
public:
    GravityPlayerComponent() = default;
    ~GravityPlayerComponent() override = default;


    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "GravityPlayerComponent"; }

    void TakeDamage(int damage);
    bool IsInvincible() const { return invincibilityTimer_ > 0.0f; }
    void SetGodMode(bool godMode) { isGodMode_ = godMode; }

    int GetHp() const { return hp_; }
    int GetMaxHp() const { return maxHp_; }
    bool IsDead() const { return isDead_; }

private:
    void HandlePullInput();
    void HandleMarkInput();
    void HandleThrowInput();
    void UpdateThrowing();

private:
    std::vector<std::shared_ptr<GameObject>> orbitingDebris_; ///< 現在プレイヤーの周囲を回転しているガレキのリスト
    int maxOrbitCount_ = 5; ///< 最大保持数
    float pullRadius_ = 100.0f; ///< 引き寄せ検知半径

    PlayerTargetingComponent* targetingComp_ = nullptr;
    class DebrisManagerComponent* debrisManager_ = nullptr;

    // Orbit parameters for pulled debris
    float orbitRadiusMin_ = 2.0f;
    float orbitRadiusMax_ = 4.0f;
    float orbitAngleRandomMax_ = 6.28f;

    bool isThrowing_ = false;
    float throwTimer_ = 0.0f;
    float hoverFrequency_ = 2.0f; // 浮遊の揺れ速度

    // 被弾処理用
    float invincibilityTimer_ = 0.0f;
    float maxInvincibilityTime_ = 1.0f;
    bool isFlashing_ = false;
    float flashTimer_ = 0.0f;
    float flashInterval_ = 0.05f;
    Irufemi::Vector4 originalBaseColor_ = {1.0f, 1.0f, 1.0f, 1.0f};
    bool colorCached_ = false;

    float throwInterval_ = 0.15f; // 0.15秒間隔
    int throwRemainingCount_ = 0; // 今回の射撃ループで撃つ弾数
    
    // ノーロック射撃時に、レイキャストが何にも当たらなかった場合の最大飛距離
    float noLockThrowDistance_ = 1000.0f; 

    // --- 体力・デバッグ関連 ---
    int hp_ = 100;
    int maxHp_ = 100;
    bool isDead_ = false;
    bool isGodMode_ = false;
};
