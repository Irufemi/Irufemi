#pragma once
#include "Combat/Boss/IBossState.h"
#include <memory>
#include <vector>

class BossDamageVisualizerComponent;
class BossComponent;

/**
 * @class IDeathSequenceStep
 * @brief ボス撃破演出の各段階（ステップ）を制御するインターフェース
 */
class IDeathSequenceStep {
public:
    virtual ~IDeathSequenceStep() = default;

    /**
     * @brief ステップ開始処理
     * @param boss 対象のボスコンポーネント
     * @param visualizer 演出コンポーネントの参照
     */
    virtual void Enter(BossComponent* boss, BossDamageVisualizerComponent* visualizer) = 0;

    /**
     * @brief 毎フレーム更新処理
     * @param boss 対象のボスコンポーネント
     * @param visualizer 演出コンポーネントの参照
     * @param dt 経過時間
     */
    virtual void Update(BossComponent* boss, BossDamageVisualizerComponent* visualizer, float dt) = 0;

    /**
     * @brief このステップの演出が完了したかどうかを判定する
     * @return 完了していればtrue
     */
    virtual bool IsCompleted() const = 0;

    /**
     * @brief ステップ終了処理
     */
    virtual void Exit(BossComponent* boss, BossDamageVisualizerComponent* visualizer) {}
};

/**
 * @class BossStateDestroyed
 * @brief ボス撃破時の演出ステート
 * @details 撃破シーケンスの各ステップ（IDeathSequenceStep）を順次実行し、演出の完了を制御します。
 */
class BossStateDestroyed : public IBossState {
public:
    /**
     * @brief ステート開始処理。撃破シーケンスステップを構築し、最初のステップを開始します。
     * @param boss 対象のボスコンポーネント
     */
    void Enter(BossComponent* boss) override;

    /**
     * @brief 毎フレーム更新処理。現在のステップを進行させ、完了時に次のステップへ遷移します。
     * @param boss 対象のボスコンポーネント
     */
    void Update(BossComponent* boss) override;

    /**
     * @brief ステート終了処理
     * @param boss 対象のボスコンポーネント
     */
    void Exit(BossComponent* boss) override;

    /**
     * @brief 被ダメージ処理（撃破済みのため無効）
     * @param boss 対象のボスコンポーネント
     * @param damage ダメージ量
     */
    void OnTakeDamage(BossComponent* boss, float damage) override;

private:
    BossDamageVisualizerComponent* visualizer_ = nullptr;   ///< 演出コンポーネントの参照
    std::vector<std::unique_ptr<IDeathSequenceStep>> steps_; ///< 演出ステップのキュー
    size_t currentStepIndex_ = 0;                           ///< 現在進行中のステップ番号
};
