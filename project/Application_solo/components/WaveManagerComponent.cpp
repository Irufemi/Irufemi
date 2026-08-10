#include "WaveManagerComponent.h"
#include "WaveEventHandlers.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "SplineFollowerComponent.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/Core/Utility/Log.h"
#include <fstream>
#include <iostream>
#include "Framework/Component/Utility/SplineComponent.h"

WaveManagerComponent::WaveManagerComponent() {}

void WaveManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Level Data Path", &levelDataPath_);
}

void WaveManagerComponent::Initialize() {
    // デフォルトハンドラの登録
    RegisterHandler("SpawnEnemy", std::make_shared<SpawnEnemyHandler>());
    RegisterHandler("PlayBGM", std::make_shared<PlayBGMHandler>());
    
    LoadLevelData(levelDataPath_);
}

void WaveManagerComponent::LoadLevelData(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        Log::OutPutLog(std::cout, "[WaveManager] Failed to load level data: " + filePath + "\n");
        return;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("Stage1_LevelData") && j["Stage1_LevelData"].contains("Events")) {
            for (const auto& eventJson : j["Stage1_LevelData"]["Events"]) {
                WaveEventData data;
                data.triggerDistance = eventJson.value("TriggerDistance", 0.0f);
                data.eventType = eventJson.value("Type", "Unknown");
                
                // パラメータとしてイベント全体を保持しておく（ハンドラ側で必要なキーを取り出す）
                data.parameters = eventJson;

                eventQueue_.push(data);
            }
        }
        Log::OutPutLog(std::cout, "[WaveManager] Loaded " + std::to_string(eventQueue_.size()) + " events from " + filePath + "\n");
    } catch (const std::exception& e) {
        Log::OutPutLog(std::cout, std::string("[WaveManager] JSON Parse Error: ") + e.what() + "\n");
    }
}

void WaveManagerComponent::RegisterHandler(const std::string& eventType, std::shared_ptr<IWaveEventHandler> handler) {
    handlers_[eventType] = handler;
}

void WaveManagerComponent::Update() {
    if (!playerFollower_) {
        // PlayerCart または Player にアタッチされている SplineFollowerComponent を探す
        auto scene = gameObject_->GetScene();
        if (scene) {
            auto cartObj = scene->FindGameObject("PlayerCart");
            if (cartObj) {
                playerFollower_ = cartObj->GetComponent<SplineFollowerComponent>();
            }
            if (!playerFollower_) {
                auto playerObj = scene->FindGameObject("Player");
                if (playerObj) {
                    playerFollower_ = playerObj->GetComponent<SplineFollowerComponent>();
                }
            }
        }
        if (!playerFollower_) return;
    }

    float currentDist = playerFollower_->GetCurrentDistance();
    auto spline = playerFollower_->GetCachedPath();

    // 進行距離が先頭イベントのトリガー距離を超えていたら発火
    while (!eventQueue_.empty()) {
        const auto& nextEvent = eventQueue_.top();
        if (currentDist >= nextEvent.triggerDistance) {
            // ハンドラを探して実行
            auto it = handlers_.find(nextEvent.eventType);
            if (it != handlers_.end() && it->second) {
                Irufemi::Vector3 pos = {0,0,0};
                Irufemi::Vector3 fwd = {0,0,1};
                Irufemi::Vector3 right = {1,0,0};
                
                if (spline) {
                    pos = spline->GetPointAtDistance(nextEvent.triggerDistance);
                    fwd = spline->GetTangentAtDistance(nextEvent.triggerDistance);
                    // 右ベクトルを計算 (上を Y軸(0,1,0) と仮定)
                    Irufemi::Vector3 up = {0.0f, 1.0f, 0.0f};
                    right = { up.y * fwd.z - up.z * fwd.y, 
                              up.z * fwd.x - up.x * fwd.z, 
                              up.x * fwd.y - up.y * fwd.x };
                    // 正規化
                    float len = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
                    if (len > 0.0001f) {
                        right.x /= len; right.y /= len; right.z /= len;
                    }
                }
                
                it->second->Execute(nextEvent, pos, fwd, right);
            } else {
                Log::OutPutLog(std::cout, "[WaveManager] Warning: No handler registered for event type: " + nextEvent.eventType + "\n");
            }

            eventQueue_.pop();
        } else {
            // Priority queue なので、先頭が条件を満たしていなければ以降も満たさない
            break;
        }
    }
}
