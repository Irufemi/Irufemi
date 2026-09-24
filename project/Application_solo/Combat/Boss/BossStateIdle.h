#pragma once
#include "Combat/Boss/IBossState.h"

/**
 * @class BossStateIdle
 * @brief ボスの通常待機（シールド展開中・定期ビーム攻撃）ステート
 * @details
 * シールドによるダメージ無効化と、定期的なビーム攻撃の発射、全シールド破壊時の露出ステートへの遷移を制御します。
 */
class BossStateIdle : public IBossState {
public:
    /**
     * @brief ステート開始処理。待機ステート開始ログを出力します。
     * @param boss 対象のボスコンポーネント
     */
    void Enter(BossComponent* boss) override;

    /**
     * @brief 毎フレーム更新処理。定期ビーム攻撃の発射タイマーと、シールド全損時のコア露出遷移を監視します。
     * @param boss 対象のボスコンポーネント
     */
    void Update(BossComponent* boss) override;

    /**
     * @brief ステート終了処理
     * @param boss 対象のボスコンポーネント
     */
    void Exit(BossComponent* boss) override;

    /**
     * @brief 被ダメージ処理。シールド展開中のためダメージをブロックします。
     * @param boss 対象のボスコンポーネント
     * @param damage ダメージ量
     */
    void OnTakeDamage(BossComponent* boss, float damage) override;
};
