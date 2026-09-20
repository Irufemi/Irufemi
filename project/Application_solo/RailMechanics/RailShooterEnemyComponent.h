#pragma once
#include "Framework/Component/Component.h"
#include "Combat/IDamageable.h"
#include <functional>

/**
 * @enum EnemyAIState
 * @brief 敵キャラクターのAI行動状態
 */
enum class EnemyAIState {
    Approach, //!< 前方定位置への進入
    Combat,   //!< 自機と一定距離を保って滞空・射撃
    Disengage //!< 制限時間終了によるすれ違い離脱
};

class GameObject;
class EnemyBulletManagerComponent;

/**
 * @class RailShooterEnemyComponent
 * @brief レールシューティング用の敵キャラクター制御コンポーネント（AIステートマシン・自機狙い射撃対応）
 */
class RailShooterEnemyComponent : public Component, public IDamageable {
public:
    RailShooterEnemyComponent() = default;
    ~RailShooterEnemyComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void OnCollisionEnter(GameObject* other) override;
    void OnRegisterProperties() override;

    std::string GetComponentName() const override {
        return "RailShooterEnemyComponent";
    }

    void TakeDamage(float damage) override {
        TakeDamage(static_cast<int>(damage));
    }
    void TakeDamage(int damage);

    DamageableType GetDamageableType() const override {
        return DamageableType::Enemy;
    }

    bool IsAlive() const {
        return hp_ > 0;
    }

    void SetOnDeathCallback(std::function<void(GameObject*)> callback) {
        onDeathCallback_ = callback;
    }

    // パラメータ設定
    void SetSpeed(float speed) {
        speed_ = speed;
    }
    void SetCombatDuration(float duration) {
        combatDuration_ = duration;
    }
    void SetShootInterval(float interval) {
        shootInterval_ = interval;
    }
    void SetTargetDistance(float dist) {
        targetDistance_ = dist;
    }
    void SetBulletScale(float scale) {
        bulletScale_ = scale;
    }
    void SetBulletSpeed(float speed) {
        bulletSpeed_ = speed;
    }

private:
    void ShootAtPlayer(const Irufemi::Vector3& playerPos);
    GameObject* GetPlayerObject();

private:
    EnemyBulletManagerComponent* bulletManager_ = nullptr; //!< キャッシュされた弾マネージャー
    EnemyAIState state_ = EnemyAIState::Approach;          //!< 現在のAIステート
    float stateTimer_ = 0.0f;                              //!< ステート内タイマー
    float combatDuration_ = 7.5f;                          //!< 滞空交戦の制限時間（秒）
    float shootInterval_ = 1.8f;                           //!< 射撃インターバル（秒）
    float shootTimer_ = 0.6f;                              //!< 射撃タイマー
    float targetDistance_ = 65.0f;                         //!< 自機前方との維持距離
    float hoverTimer_ = 0.0f;                              //!< 浮遊サイン波タイマー
    int bodyDamage_ = 20;                                  //!< 体当たり衝突ダメージ
    float bulletScale_ = 0.3f;                             //!< 敵弾のスケール・コライダー半径
    float bulletSpeed_ = 32.0f;                            //!< 敵弾の飛翔速度

    float spawnProgress_ = 0.5f; //!< プレイヤーがどの進行度に達したらアクティブになるか (0.0 ~ 1.0)
    bool isActive_ = false;      //!< 現在活動中かどうか
    float speed_ = 15.0f;        //!< 敵の進入・離脱速度
    int hp_ = 100;               //!< 耐久力

    std::function<void(GameObject*)> onDeathCallback_;
};
