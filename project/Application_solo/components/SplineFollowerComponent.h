#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <memory>
#include <string>

class SplineComponent;
class Line3DBatch;

/**
 * @class SplineFollowerComponent
 * @brief 指定されたスプラインレール上を移動し、自身のTransformを更新するコンポーネント
 */
class SplineFollowerComponent : public Component {
public:
    SplineFollowerComponent() = default;
    ~SplineFollowerComponent() override = default;

    void Initialize() override;
    void Start() override;
    void Update() override;
    void Draw() override;
    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "SplineFollowerComponent"; }

    float GetCurrentDistance() const { return currentDistance_; }
    SplineComponent* GetCachedPath() const { return cachedPath_; }

private:
    float currentDistance_ = 0.0f;    ///< ルート（軌道）上の進み具合 (m)
    float speed_ = 10.0f;             ///< 自動前進するスピード (m/s)
    std::string targetPathName_ = "PathManager"; ///< 追従対象のオブジェクト名
    SplineComponent* cachedPath_ = nullptr; ///< キャッシュされた対象のパス
};
