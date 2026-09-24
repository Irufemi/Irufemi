#pragma once
#include "Combat/Boss/IBossState.h"

/**
 * @class BossStateCoreExposed
 * @brief ボスのコア露出（弱点露出・ダメージ受付）ステート
 * @details 全てのシールドが破壊された後に遷移し、プレイヤーからのダメージ受付と被弾時カメラシェイク演出を制御します。
 */
class BossStateCoreExposed : public IBossState {
public:
    /**
     * @brief ステート開始処理。コア露出ログを出力します。
     * @param boss 対象のボスコンポーネント
     */
    void Enter(BossComponent* boss) override;

    /**
     * @brief 毎フレーム更新処理
     * @param boss 対象のボスコンポーネント
     */
    void Update(BossComponent* boss) override;

    /**
     * @brief ステート終了処理
     * @param boss 対象のボスコンポーネント
     */
    void Exit(BossComponent* boss) override;

    /**
     * @brief 被ダメージ処理。HPを減算しカメラシェイクを再生、HPが0以下で撃破ステートへ遷移します。
     * @param boss 対象のボスコンポーネント
     * @param damage ダメージ量
     */
    void OnTakeDamage(BossComponent* boss, float damage) override;

    /**
     * @brief コア露出中フラグを返す
     * @return 常に true
     */
    bool IsCoreExposed() const override {
        return true;
    }
};
