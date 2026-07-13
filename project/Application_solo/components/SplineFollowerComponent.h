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

private:
    float progress_ = 0.0f;           ///< ルート（軌道）上の進み具合 (0.0〜1.0)
    float speed_ = 0.05f;             ///< 自動前進するスピード (1秒間に進む割合)
    std::string targetPathName_ = "PathManager"; ///< 追従対象のオブジェクト名
    bool drawDebugRail_ = false;      ///< デバッグ用にレールの軌道を描画するかどうか

    SplineComponent* cachedPath_ = nullptr; ///< キャッシュされた対象のパス
    std::unique_ptr<Line3DBatch> debugLineBatch_; ///< デバッグ描画用のラインバッチ
};
