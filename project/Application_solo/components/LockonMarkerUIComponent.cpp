#include "LockonMarkerUIComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Graphics/Pipeline/PSOManager.h"
#include "Engine/Core/Utility/Ease.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/Graphics/Camera/Camera.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Utility/ErrorUtility.h"
#include <windows.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <unordered_map>

void LockonMarkerUIComponent::Initialize() {
    auto engine = BaseModel::GetIrufemiEngine();
    IRUFEMI_ASSERT(engine != nullptr && "IrufemiEngine is null in Initialize");

    // 1. SpriteBatch の生成（テクスチャ指定）
    markerBatch_ = std::make_unique<SpriteBatch>();
    markerBatch_->Initialize("resources/reticle.jpg");

    // 2. カスタムシェーダー（輝度アルファ抜き）の登録・取得と適用
    auto psoManager = engine->GetPSOManager();
    IRUFEMI_ASSERT(psoManager != nullptr && "PSOManager is null");

    // 2D用のブレンドモード(通常透過: Normal)を指定
    auto pso = psoManager->GetPSO("LuminanceAlpha2D", BlendMode::kBlendModeNormal, PSOManager::DepthWrite::Off, PSOManager::CullMode::None);
    IRUFEMI_ASSERT(pso != nullptr && "LuminanceAlpha2D PSO not found!");
    markerBatch_->SetCustomPSO(pso);
}

void LockonMarkerUIComponent::SyncTargets(const std::vector<std::shared_ptr<GameObject>>& targets) {
    std::vector<LockonMarkerState> nextMarkers;
    std::unordered_map<GameObject*, int> targetCounts;

    for (const auto& target : targets) {
        if (!target) continue;

        int occurrenceIndex = targetCounts[target.get()]++;

        bool found = false;
        int activeOccurrence = 0;
        for (const auto& active : activeMarkers_) {
            if (active.target.lock() == target) {
                if (activeOccurrence == occurrenceIndex) {
                    nextMarkers.push_back(active);
                    found = true;
                    break;
                }
                activeOccurrence++;
            }
        }

        if (!found) {
            // 新規ロックオン対象
            LockonMarkerState newState;
            newState.target = target;
            newState.currentScale = 3.0f; // 初期スケール（大きめに出現）
            newState.animationT = 0.0f;
            nextMarkers.push_back(newState);
        }
    }

    activeMarkers_ = std::move(nextMarkers);
}

void LockonMarkerUIComponent::Update() {
    IRUFEMI_ASSERT(markerBatch_ != nullptr && "markerBatch_ is null in Update");

    // バッチへの登録をリセット
    markerBatch_->ClearInstances();

    auto engine = BaseModel::GetIrufemiEngine();
    IRUFEMI_ASSERT(engine != nullptr && "IrufemiEngine is null");
    float deltaTime = engine->GetDeltaTime();
    
    auto cameraManager = engine->GetCameraManager();
    IRUFEMI_ASSERT(cameraManager != nullptr && "cameraManager is null");
    auto camera = cameraManager->GetActiveCamera();
    IRUFEMI_ASSERT(camera != nullptr && "camera is null");

    // デバッグログが大量に出るのを防ぐため、60フレームに1回だけ出力
    static int frameCounter = 0;
    bool shouldLog = (frameCounter++ % 60 == 0);

    if (shouldLog) {
        std::string logMsg = "[LockonMarkerUI] Active Markers: " + std::to_string(activeMarkers_.size()) + "\n";
        OutputDebugStringA(logMsg.c_str());
    }

    int markerIndex = 0;
    std::unordered_map<GameObject*, int> drawCounts;

    for (auto& marker : activeMarkers_) {
        auto target = marker.target.lock();
        if (!target) continue;

        auto transform = target->GetComponent<TransformComponent>();
        if (!transform) continue;

        // 3D座標から2Dスクリーン座標への手動変換（Zは0.0～1.0の深度）
        Vector3 worldPos = transform->GetWorldPosition();
        Matrix4x4 viewProj = camera->GetViewProjectionMatrix3D();
        Vector3 clipPos = Math::Transform(worldPos, viewProj);
        
        Vector3 screenPos;
        screenPos.z = clipPos.z;
        screenPos.x = (clipPos.x + 1.0f) * 0.5f * camera->GetViewportWidth();
        screenPos.y = (1.0f - clipPos.y) * 0.5f * camera->GetViewportHeight();
        
        if (shouldLog) {
            char buf[256];
            snprintf(buf, sizeof(buf), "[LockonMarkerUI] Marker %d: Target %p, WorldPos(%.1f, %.1f, %.1f), ScreenPos(%.1f, %.1f, %.3f), Scale %.2f\n",
                     markerIndex, target.get(), worldPos.x, worldPos.y, worldPos.z, screenPos.x, screenPos.y, screenPos.z, marker.currentScale);
            OutputDebugStringA(buf);
        }

        // 画面奥に行っている場合はスキップ（Z > 1.0 または Z < 0.0）
        if (screenPos.z >= 1.0f || screenPos.z <= 0.0f) {
            if (shouldLog) {
                OutputDebugStringA("[LockonMarkerUI] -> Clipped due to Z value out of range (0.0 - 1.0)\n");
            }
            continue;
        }

        // 距離に応じた基本スケールの計算（遠いほど小さく）
        float distanceScale = std::clamp(1.0f - screenPos.z, 0.2f, 1.0f);
        marker.targetScale = distanceScale;

        // アニメーション（イージング）の進行
        if (marker.animationT < 1.0f) {
            marker.animationT += deltaTime * 5.0f; // アニメーション速度 (約0.2秒で完了)
            if (marker.animationT > 1.0f) marker.animationT = 1.0f;
        }

        // EaseOutCubic を使ってシュッと縮小するアニメーション
        float easedT = EaseOutCubic(marker.animationT);
        marker.currentScale = std::lerp(3.0f, marker.targetScale, easedT);

        int idx = drawCounts[target.get()]++;
        float finalScale = marker.currentScale;
        Vector2 finalPos = { screenPos.x, screenPos.y };

        if (idx > 0) {
            // サテライト配置
            // 最大5発なのでサテライトは最大4つ（十字または四角形の配置になる）
            float angle = (idx - 1) * (6.283185f / 4.0f) + (engine->GetTotalTime() * 1.5f); // クルクル回す
            float offset = 50.0f * marker.targetScale; // 距離に応じてオフセットも縮める
            finalPos.x += std::cos(angle) * offset;
            finalPos.y += std::sin(angle) * offset;
            
            finalScale *= 0.6f; // サテライトは少し小さくする
        }

        // バッチにインスタンスを追加 (SpriteBatchのAddInstanceはVector2, Vector2, rotation, color, anchor)
        float size = 100.0f * finalScale;
        markerBatch_->AddInstance(finalPos, Vector2{size, size}, 0.1f, Vector4{1.0f, 1.0f, 1.0f, 1.0f});
        
        markerIndex++;
    }

    // インスタンスバッファを構築（これを呼ばないとGPUにデータが送られない）
    markerBatch_->Update();

    // 描画キューにバッチを登録する（これを呼ばないと描画されない）
    markerBatch_->Draw();
}
