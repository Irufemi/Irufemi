#include "Level/WaveManagerComponent.h"
#include "Level/WaveEventHandlers.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "RailMechanics/SplineFollowerComponent.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/Utility/Log.h"
#include <fstream>
#include <iostream>
#include "Framework/Component/Utility/SplineComponent.h"
#include "Framework/Component/Logic/SpawnPointComponent.h"

WaveManagerComponent::WaveManagerComponent() {}

void WaveManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Level Data Path", &levelDataPath_);
}

void WaveManagerComponent::Initialize() {
    // デフォルトハンドラの登録
    RegisterHandler("SpawnEnemy", std::make_shared<SpawnEnemyHandler>());
    RegisterHandler("PlayBGM", std::make_shared<PlayBGMHandler>());
    
    ReloadLevelData();
}

void WaveManagerComponent::ReloadLevelData() {
    std::priority_queue<WaveEventData, std::vector<WaveEventData>, std::greater<WaveEventData>> emptyQueue;
    std::swap(eventQueue_, emptyQueue);
    allEvents_.clear();
    
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
                
                data.parameters = eventJson;

                eventQueue_.push(data);
                allEvents_.push_back(data);
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
    if (!hasCachedSpawnPoints_) {
        CacheSpawnPoints();
        hasCachedSpawnPoints_ = true;
    }

#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
    // エディタモード中、自身が選択されている時のみプレビューを描画する
    auto engine = BaseModel::GetIrufemiEngine();
    if (engine && engine->GetSelectedObject().get() == gameObject_) {
        // キャッシュの再構築（SpawnPointがエディタ上で移動・追加されている可能性を考慮して毎フレーム更新）
        CacheSpawnPoints();

        auto scene = gameObject_->GetScene();
        auto cartObj = scene ? scene->FindGameObject("PlayerCart") : nullptr;
        if (!cartObj && scene) cartObj = scene->FindGameObject("Player");
        
        auto follower = cartObj ? cartObj->GetComponent<SplineFollowerComponent>() : nullptr;
        auto spline = follower ? follower->GetCachedPath() : nullptr;

        if (spline) {
            for (const auto& ev : allEvents_) {
                auto it = handlers_.find(ev.eventType);
                if (it != handlers_.end() && it->second) {
                    Irufemi::Vector3 pos = spline->GetPointAtDistance(ev.triggerDistance);
                    Irufemi::Vector3 fwd = spline->GetTangentAtDistance(ev.triggerDistance);
                    Irufemi::Vector3 up = {0.0f, 1.0f, 0.0f};
                    Irufemi::Vector3 right = { up.y * fwd.z - up.z * fwd.y, 
                                               up.z * fwd.x - up.x * fwd.z, 
                                               up.x * fwd.y - up.y * fwd.x };
                    float len = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
                    if (len > 0.0001f) { right.x /= len; right.y /= len; right.z /= len; }
                    
                    it->second->DrawEditorPreview(this, ev, pos, fwd, right);
                }
            }
        }
    }
#endif

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
                
                it->second->Execute(this, nextEvent, pos, fwd, right);
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

void WaveManagerComponent::CacheSpawnPoints() {
    auto scene = gameObject_->GetScene();
    if (!scene) return;

    spawnPointsMap_.clear();
    const auto& objs = scene->GetGameObjects();
    for (const auto& obj : objs) {
        if (!obj) continue;
        
        // 最初のバグ (this == 0xF) 対策: 生ポインタが異常に小さい値かどうかをチェック
        auto* rawPtr = obj.get();
        if (reinterpret_cast<uintptr_t>(rawPtr) < 0x1000) {
            Log::OutPutLog(std::cerr, "[WaveManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" + 
                           std::format("{:X}", reinterpret_cast<uintptr_t>(rawPtr)) + ") in CacheSpawnPoints!\n");
            continue;
        }

        if (!obj->GetIsActive()) continue;
        auto sp = obj->GetComponent<SpawnPointComponent>();
        if (sp) {
            spawnPointsMap_[sp->GetWaveId()].push_back(sp);
        }
    }
}

const std::vector<SpawnPointComponent*>& WaveManagerComponent::GetSpawnPoints(const std::string& waveId) const {
    static const std::vector<SpawnPointComponent*> emptyList;
    auto it = spawnPointsMap_.find(waveId);
    if (it != spawnPointsMap_.end()) {
        return it->second;
    }
    return emptyList;
}
