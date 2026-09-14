#pragma once
#include "Framework/Component/Component.h"
#include <vector>
#include <functional>

/**
 * @class TargetableComponent
 * @brief プレイヤーや各種システムからロックオン・ターゲット指定可能なオブジェクトに付与するコンポーネント
 * @details シーン内のターゲット可能オブジェクトを一括管理し、ロックオン判定時の候補を提供します。
 *          敵や障害物など、ターゲット条件が動的に変化するオブジェクトは SetTargetablePredicate を用いて
 *          ターゲット可否の判定ロジックを登録できます。
 */
class TargetableComponent : public Component {
public:
    using TargetablePredicate = std::function<bool()>;

    TargetableComponent() = default;
    ~TargetableComponent() override;

    void OnEnable() override;
    void OnDisable() override;

    std::string GetComponentName() const override {
        return "TargetableComponent";
    }

    /**
     * @brief 登録されているすべての TargetableComponent を取得する
     * @return ターゲット候補コンポーネントのリスト
     */
    static const std::vector<TargetableComponent*>& GetTargets() {
        return s_targets;
    }

    /**
     * @brief ターゲット可能かどうかを判定する外部述語関数を設定する
     * @param predicate ターゲット可能ならtrueを返す述語関数
     */
    void SetTargetablePredicate(TargetablePredicate predicate) {
        predicate_ = std::move(predicate);
    }

    /**
     * @brief 現在このオブジェクトがターゲット可能かどうかを判定する
     * @return ターゲット可能ならtrue
     */
    bool IsTargetable() const;

private:
    static std::vector<TargetableComponent*> s_targets;
    TargetablePredicate predicate_ = nullptr;
};

