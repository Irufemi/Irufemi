#pragma once

class BossComponent;

class IBossState {
public:
    virtual ~IBossState() = default;

    /**
     * @brief 状態に遷移した最初のフレームで呼ばれる
     */
    virtual void Enter(BossComponent* boss) = 0;

    /**
     * @brief 毎フレーム呼ばれる
     */
    virtual void Update(BossComponent* boss) = 0;

    /**
     * @brief 状態から抜ける際に呼ばれる
     */
    virtual void Exit(BossComponent* boss) = 0;

    /**
     * @brief ボスがダメージを受けた際に呼ばれる
     */
    virtual void OnTakeDamage(BossComponent* boss, float damage) {}

    /**
     * @brief コアが露出している（ダメージを受け付ける/ターゲット可能）状態かどうか
     */
    virtual bool IsCoreExposed() const {
        return false;
    }
};
