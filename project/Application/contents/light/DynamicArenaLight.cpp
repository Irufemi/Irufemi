#include "DynamicArenaLight.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Graphics/Data/AreaLight.h"
#include <algorithm>

void DynamicArenaLight::Initialize(IrufemiEngine* engine, std::vector<std::unique_ptr<AreaLight>>& sceneAreaLights) {
    engine_ = engine;

    auto aLight = std::make_unique<AreaLight>();
    aLight->color = {1.0f, 0.9f, 0.8f, 1.0f}; // やや暖色寄りの白
    aLight->intensity = kBaseIntensity;
    aLight->range = kMinLightRange;
    aLight->size = {kLightSizeWidth, kLightSizeHeight};
    aLight->direction = {0.0f, -1.0f, 0.001f}; // 真下付近を向かせる (0.001fは外積時の0除算防止)
    aLight->isActive = 1;

    // 制御対象の参照を保持
    controlledLight_ = aLight.get();

    // シーンのライトリストに登録
    sceneAreaLights.push_back(std::move(aLight));
}

void DynamicArenaLight::Update(const Vector3& playerPos, const Vector3& enemyPos) {
    if (!controlledLight_ || !engine_) return;

    controlledLight_->isActive = 1;

    // 1. ライトの位置を計算 (2人の中間点の上空)
    Vector3 midPos = Math::Add(playerPos, enemyPos);
    midPos.x *= 0.5f;
    midPos.z *= 0.5f;
    midPos.y = kFixedLightHeight;

    controlledLight_->position = midPos;

    // 2. 距離に基づく影響範囲(range)と強度(intensity)の計算
    float distance = Math::Length(Math::Subtract(enemyPos, playerPos));
    controlledLight_->range = (std::max)(kMinLightRange, distance * kLightRangeDistanceFactor);
    controlledLight_->intensity = kBaseIntensity + (distance * kIntensityDistanceFactor);

    // 3. シャドウマップパラメータの計算
    Vector3 shadowTargetPos = midPos;
    // Y軸は低い方に合わせることで影が途切れるのを防ぐ
    shadowTargetPos.y = (std::min)(playerPos.y, enemyPos.y); 

    float shadowOrthoSize = (std::max)(kMinShadowOrthoSize, distance * kShadowOrthoDistanceFactor + kShadowOrthoMargin);

    // エンジンにシャドウパラメータを送信
    engine_->GetDrawManager()->SetShadowParameters(shadowTargetPos, shadowOrthoSize);
}
