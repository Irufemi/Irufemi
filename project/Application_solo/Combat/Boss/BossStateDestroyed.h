#pragma once
#include "Combat/Boss/IBossState.h"
#include <memory>

class CameraShakeComponent;

/**
 * @class BossStateDestroyed
 * @brief ボス撃破時の演出ステート
 * @details 画面全体の特大シェイクとモデル非表示、および死亡完了通知を制御します。
 */
class BossStateDestroyed : public IBossState {
public:
    /**
     * @brief ステート開始処理。ボス撃破コールバックの発行とカメラシェイクを開始します。
     * @param boss 対象のボスコンポーネント
     */
    void Enter(BossComponent* boss) override;

    /**
     * @brief 毎フレーム更新処理。カメラシェイクの終了を監視し、死亡シーケンス完了を通知します。
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
    std::weak_ptr<class GameObject> cameraObj_; ///< カメラオブジェクトへの安全な弱参照
    bool hasFinished_ = false;                  ///< 撃破演出終了フラグ
};
