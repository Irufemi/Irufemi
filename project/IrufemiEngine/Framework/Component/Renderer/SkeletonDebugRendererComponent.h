#pragma once

#include "../Component.h"
#include <memory>
#include <string>

class PrimitiveBatch;
class Line3DBatch;

/**
 * @class SkeletonDebugRendererComponent
 * @brief スケルトンのデバッグ表示を行うコンポーネント。
 * @details 同じGameObjectにアタッチされている SkinnedMeshRendererComponent から
 *          SkeletonPose を取得し、ジョイント間の接続（八面体）やローカルXYZ軸（線分）を描画します。
 */
class SkeletonDebugRendererComponent : public Component {
public:
    SkeletonDebugRendererComponent();
    ~SkeletonDebugRendererComponent() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    bool CanUpdateInEditMode() const override { return true; }

    void OnRegisterProperties() override;
    std::string GetComponentName() const override { return "SkeletonDebugRendererComponent"; }

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    void SetShowBones(bool show) { showBones_ = show; }
    void SetShowAxes(bool show) { showAxes_ = show; }

private:
    std::unique_ptr<PrimitiveBatch> boneMeshes_;
    std::unique_ptr<Line3DBatch> debugAxesLines_;

    bool showBones_ = true;
    bool showAxes_ = true;
    float axisScale_ = 1.0f;
};
