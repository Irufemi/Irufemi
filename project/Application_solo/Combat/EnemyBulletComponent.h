#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"

class GameObject;

/**
 * @class EnemyBulletComponent
 * @brief 敵が発射する自機狙い弾の移動・寿命・プレイヤー衝突判定を制御するコンポーネント
 */
class EnemyBulletComponent : public Component {
public:
    EnemyBulletComponent() = default;
    ~EnemyBulletComponent() override = default;

    void Initialize() override;
    void Update() override;
    void OnCollisionEnter(GameObject* other) override;

    std::string GetComponentName() const override {
        return "EnemyBulletComponent";
    }

    /**
     * @brief 弾の進行方向と速度、威力を設定して射出する
     * @param direction 射出方向の単位ベクトル
     * @param speed 移動速度（デフォルト 30.0f）
     * @param damage プレイヤーへの被弾ダメージ（デフォルト 10）
     */
    void Launch(const Irufemi::Vector3& direction, float speed = 30.0f, int damage = 10);

private:
    Irufemi::Vector3 velocity_{0.0f, 0.0f, 0.0f}; //!< 移動速度ベクトル
    int damage_ = 10;                             //!< 命中時のダメージ量
    float lifeTimer_ = 0.0f;                      //!< 生存タイマー
    float maxLifeTime_ = 5.0f;                    //!< 最大寿命（秒）
};
