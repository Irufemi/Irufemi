#pragma once

enum class DamageableType {
    Enemy,
    Boss,
    Environment,
    Player
};

/**
 * @class IDamageable
 * @brief ダメージを受けることが可能なゲームオブジェクト用インターフェース
 */
class IDamageable {
public:
    virtual ~IDamageable() = default;

    /**
     * @brief ダメージを与える
     * @param damage 与えるダメージ量
     */
    virtual void TakeDamage(float damage) = 0;

    /**
     * @brief ダメージ対象の種別を取得する
     * @return DamageableType 種別
     */
    virtual DamageableType GetDamageableType() const {
        return DamageableType::Enemy;
    }
};
