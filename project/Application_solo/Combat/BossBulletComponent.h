#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"

class BossBulletManagerComponent;

/**
 * @class BossBulletComponent
 * @brief ボス（およびドローン）から発射されるプラズマ弾。
 *        ガレキとの相殺判定やプレイヤーへの被弾判定を行う。
 */
class BossBulletComponent : public Component {
public:
    BossBulletComponent();
    ~BossBulletComponent() override = default;

    void Update() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "BossBulletComponent"; }

    /**
     * @brief 弾を発射する
     * @param manager 自身を管理しているマネージャー（回収時に通知するため）
     * @param startPos 発射初期座標
     * @param velocity 進行方向と速度
     * @param lifeTime 生存時間
     */
    void Shoot(BossBulletManagerComponent* manager, const Irufemi::Vector3& startPos, const Irufemi::Vector3& velocity, float lifeTime = 5.0f);

    /**
     * @brief プレイヤーの投げたガレキなどに命中し、相殺された時の処理
     */
    void OnIntercepted();

private:
    BossBulletManagerComponent* manager_ = nullptr;
    Irufemi::Vector3 velocity_ = {0.0f, 0.0f, 0.0f};
    float lifeTimer_ = 0.0f;
    bool isActiveBullet_ = false;
};
