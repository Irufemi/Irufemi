#pragma once
#include "Framework/Component/Component.h"
#include "Core/Math/Vector3.h"
#include <string>
#include <memory>

class GameObject;
class SplineFollowerComponent;
class SplineComponent;

/**
 * @brief レール上の指定ターゲット（Player等）から、一定の距離（Zオフセット）を保って追従するコンポーネント。
 *        ボスなどがプレイヤーの前方を一定距離保ったまま並走・逃走する挙動を実現する。
 */
class RailRelativeFollowerComponent : public Component {
public:
    RailRelativeFollowerComponent() = default;
    ~RailRelativeFollowerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;

    void OnRegisterProperties() override;
    std::string GetComponentName() const override {
        return "RailRelativeFollowerComponent";
    }

    void OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) override;

private:
    uint64_t targetObjectID_ = 0;  ///< 追従する基準となるターゲットのオブジェクトID
    float distanceOffset_ = 80.0f; ///< ターゲットからどれだけ前方に離れるか (m)
    Irufemi::Vector3 localOffset_ = {0.0f, 0.0f, 0.0f}; ///< レール中心からのXYローカルオフセット

    std::weak_ptr<GameObject> targetObject_;
    SplineFollowerComponent* targetFollower_ = nullptr;
    SplineComponent* cachedPath_ = nullptr; ///< ターゲットが乗っているレール（スプライン）
};
