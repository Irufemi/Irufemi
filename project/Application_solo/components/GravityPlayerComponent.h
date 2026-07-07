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

    bool isThrowing_ = false;
    float throwTimer_ = 0.0f;
    float throwInterval_ = 0.15f; // 0.15秒間隔
    int throwRemainingCount_ = 0; // 今回の射撃ループで撃つ弾数
    
    // ノーロック射撃時に、レイキャストが何にも当たらなかった場合の最大飛距離
    float noLockThrowDistance_ = 1000.0f; 
};
