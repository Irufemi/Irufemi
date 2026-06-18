#pragma once
#include "Framework/Component/Component.h"
#include "Engine/Core/Math/Vector3.h"
#include <vector>
#include <string>

/**
 * @class SplineComponent
 * @brief 複数のウェイポイント間を Catmull-Rom スプラインで補間し、座標や接線を取得する汎用コンポーネント
 */
class SplineComponent : public Component {
public:
    SplineComponent() = default;
    ~SplineComponent() override = default;

    void OnRegisterProperties() override;

    std::string GetComponentName() const override { return "SplineComponent"; }

    /**
     * @brief 0.0 ~ 1.0 の進行度(t)から、スプライン上の座標を取得する
     * @param t 進行度 (0.0=開始地点, 1.0=終了地点)
     * @return 補間された座標
     */
    Vector3 GetPointAt(float t) const;

    /**
     * @brief 指定した進行度(t)における接線（進行方向）ベクトルを取得する
     * @param t 進行度 (0.0~1.0)
     * @return 正規化された接線ベクトル
     */
    Vector3 GetTangentAt(float t) const;

    // ウェイポイントのリストを取得・設定
    const std::vector<Vector3>& GetWaypoints() const { return waypoints_; }
    void SetWaypoints(const std::vector<Vector3>& points) { waypoints_ = points; }

private:
    std::vector<Vector3> waypoints_; ///< スプラインの制御点
};
