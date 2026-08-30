#include "Framework/Component/Utility/SplineNodeComponent.h"
#include "Core/Shape/LinePrimitive.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include <cmath>

void SplineNodeComponent::OnRegisterProperties() {
    RegisterPropertyRange("Radius", &radius_, 0.1f, 10.0f);
    RegisterProperty("Draw Debug", &drawDebug_);
}

void SplineNodeComponent::Draw() {
    if (!drawDebug_ || !gameObject_)
        return;

    auto transform = GetTransform();
    if (!transform)
        return;

    auto scene = gameObject_->GetScene();
    if (!scene)
        return;
    auto engine = scene->GetEngine();
    if (!engine)
        return;
    auto debugRenderer = engine->GetDebugPrimitiveRenderer();
    if (debugRenderer) {
        debugRenderer->AddSphere(transform->GetWorldPosition(), radius_, color_);
    }
}

bool SplineNodeComponent::Raycast(const Irufemi::Ray& ray, float& outDistance) const {
    if (!gameObject_)
        return false;

    auto transform = GetTransform();
    if (!transform)
        return false;

    Irufemi::Vector3 center = transform->GetWorldPosition();

    // Rayと球の交差判定 (線分・直線の交点計算)
    Irufemi::Vector3 m = {ray.origin.x - center.x, ray.origin.y - center.y, ray.origin.z - center.z};

    float b = m.x * ray.diff.x + m.y * ray.diff.y + m.z * ray.diff.z;
    float c = (m.x * m.x + m.y * m.y + m.z * m.z) - radius_ * radius_;

    // 始点が球の外側にあり、レイが球から遠ざかっている場合は交差しない
    if (c > 0.0f && b > 0.0f)
        return false;

    float discr = b * b - c;
    // 判別式が負の場合は交差しない
    if (discr < 0.0f)
        return false;

    // 交差距離を計算 (近い方の交点)
    float t = -b - std::sqrt(discr);

    // 始点が球の内部にある場合は、もう一方の交点を採用
    if (t < 0.0f)
        t = 0.0f;

    outDistance = t;
    return true;
}
