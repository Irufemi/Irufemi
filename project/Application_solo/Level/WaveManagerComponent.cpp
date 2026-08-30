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
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include "Core/Math/MathFunction.h"
#include "Renderer/Object/3D/StaticModelObject/StaticModelObject.h"
#include "Resource/Model/ModelManager.h"
#include "Framework/Component/Logic/SpawnPointComponent.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"

WaveManagerComponent::WaveManagerComponent() {}

void WaveManagerComponent::OnRegisterProperties() {
    Component::OnRegisterProperties();
    RegisterProperty("Level Data Path", &levelDataPath_);
    RegisterGameObjectRef("Target Spline", &targetSplineID_);
    RegisterProperty("Editor Preview Distance", &editorPreviewDistance_);
}

void WaveManagerComponent::Deserialize(const nlohmann::json& j) {
    Component::Deserialize(j);
    ReloadLevelData();
}

void WaveManagerComponent::Initialize() {
    // デフォルトハンドラの登録
    RegisterHandler("SpawnEnemy", std::make_shared<SpawnEnemyHandler>());
    RegisterHandler("PlayBGM", std::make_shared<PlayBGMHandler>());

    ReloadLevelData();
}

std::shared_ptr<ModelBatchRendererComponent>
WaveManagerComponent::GetPreviewBatchRenderer(const std::string& modelPath) {
    auto engine = BaseModel::GetIrufemiEngine();
    if (!engine)
        return nullptr;

    if (currentPreviewModelPath_ != modelPath || !previewBatch_) {
        currentPreviewModelPath_ = modelPath;
        if (!previewBatch_) {
            previewBatch_ = std::make_shared<ModelBatchRendererComponent>();
            previewBatch_->SetGameObject(gameObject_);
            previewBatch_->SetUseGPUCulling(false); // プレビュー用なのでカリング無効化
            previewBatch_->Initialize();
        }
        previewBatch_->LoadModel(modelPath);
    }

    return previewBatch_;
}

void WaveManagerComponent::Draw() {
#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
    auto engine = BaseModel::GetIrufemiEngine();
    if (engine && engine->GetSelectedObject().get() == gameObject_) {
        auto scene = gameObject_->GetScene();
        if (!scene)
            return;

        if (previewBatch_) {
            previewBatch_->ClearInstances();
        }

        SplineComponent* spline = nullptr;
        if (targetSplineID_ != 0) {
            auto splineObj = scene->FindGameObjectByID(targetSplineID_);
            if (splineObj)
                spline = splineObj->GetComponent<SplineComponent>();
        }

        if (spline) {
            if (engine->GetDebugPrimitiveRenderer()) {
                Irufemi::Vector3 phPos = spline->GetPointAtDistance(editorPreviewDistance_);
                Irufemi::Vector3 scale = {3.0f, 3.0f, 3.0f};
                Irufemi::Matrix4x4 transform =
                    Irufemi::Math::MakeAffineMatrix(scale, Irufemi::Vector3{0.0f, 0.0f, 0.0f}, phPos);
                Irufemi::Vector4 color = {1.0f, 1.0f, 0.0f, 1.0f}; // Yellow for Playhead
                engine->GetDebugPrimitiveRenderer()->AddCube(transform, color);
            }
        }

        auto cartObj = scene->FindGameObject("PlayerCart");
        if (!cartObj)
            cartObj = scene->FindGameObject("Player");
        auto follower = cartObj ? cartObj->GetComponent<SplineFollowerComponent>() : nullptr;
        SplineComponent* railSpline = nullptr;
        if (follower) {
            if (auto pathObj = scene->FindGameObjectByID(follower->GetTargetPathID())) {
                railSpline = pathObj->GetComponent<SplineComponent>();
            }
        }

        if (railSpline) {
            for (const auto& ev : allEvents_) {
                // GPU負荷軽減のため、現在のプレイヘッド距離から遠すぎるイベントはプレビュー描画をスキップする
                if (std::abs(ev.triggerDistance - editorPreviewDistance_) > 300.0f) {
                    continue;
                }

                auto it = handlers_.find(ev.eventType);
                if (it != handlers_.end() && it->second) {
                    Irufemi::Vector3 pos = railSpline->GetPointAtDistance(ev.triggerDistance);
                    Irufemi::Vector3 fwd = railSpline->GetTangentAtDistance(ev.triggerDistance);
                    Irufemi::Vector3 up = {0.0f, 1.0f, 0.0f};
                    Irufemi::Vector3 right = {up.y * fwd.z - up.z * fwd.y, up.z * fwd.x - up.x * fwd.z,
                                              up.x * fwd.y - up.y * fwd.x};
                    float len = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
                    if (len > 0.0001f) {
                        right.x /= len;
                        right.y /= len;
                        right.z /= len;
                    }

                    it->second->DrawEditorPreview(this, ev, pos, fwd, right);
                }
            }
        }

        if (previewBatch_) {
            previewBatch_->Draw();
        }
    }
#endif
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
        Log::OutPutLog(std::cout, "[WaveManager] Loaded " + std::to_string(eventQueue_.size()) + " events from " +
                                      filePath + "\n");
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

    auto engine = BaseModel::GetIrufemiEngine();
    bool isPlayMode = engine && engine->IsPlayMode();
    if (!isPlayMode) {
        return; // エディタモードではイベントの消費とスポーンを行わない
    }

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
        if (!playerFollower_)
            return;
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
                Irufemi::Vector3 pos = {0, 0, 0};
                Irufemi::Vector3 fwd = {0, 0, 1};
                Irufemi::Vector3 right = {1, 0, 0};

                if (spline) {
                    pos = spline->GetPointAtDistance(nextEvent.triggerDistance);
                    fwd = spline->GetTangentAtDistance(nextEvent.triggerDistance);
                    // 右ベクトルを計算 (上を Y軸(0,1,0) と仮定)
                    Irufemi::Vector3 up = {0.0f, 1.0f, 0.0f};
                    right = {up.y * fwd.z - up.z * fwd.y, up.z * fwd.x - up.x * fwd.z, up.x * fwd.y - up.y * fwd.x};
                    // 正規化
                    float len = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
                    if (len > 0.0001f) {
                        right.x /= len;
                        right.y /= len;
                        right.z /= len;
                    }
                }

                it->second->Execute(this, nextEvent, pos, fwd, right);
            } else {
                Log::OutPutLog(std::cout, "[WaveManager] Warning: No handler registered for event type: " +
                                              nextEvent.eventType + "\n");
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
    if (!scene)
        return;

    spawnPointsMap_.clear();
    const auto& objs = scene->GetGameObjects();
    for (const auto& obj : objs) {
        if (!obj)
            continue;

        // 最初のバグ (this == 0xF) 対策: 生ポインタが異常に小さい値かどうかをチェック
        auto* rawPtr = obj.get();
        if (reinterpret_cast<uintptr_t>(rawPtr) < 0x1000) {
            Log::OutPutLog(std::cerr, "[WaveManager] CRITICAL ERROR: Caught invalid GameObject pointer (0x" +
                                          std::format("{:X}", reinterpret_cast<uintptr_t>(rawPtr)) +
                                          ") in CacheSpawnPoints!\n");
            continue;
        }

        if (!obj->GetIsActive())
            continue;
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

void WaveManagerComponent::OnIDRemapped(const std::unordered_map<uint64_t, uint64_t>& idMap) {
    if (targetSplineID_ != 0) {
        auto it = idMap.find(targetSplineID_);
        if (it != idMap.end()) {
            targetSplineID_ = it->second;
        }
    }
}

void WaveManagerComponent::SaveLevelData() {
    SaveLevelData(levelDataPath_);
}

void WaveManagerComponent::SaveLevelData(const std::string& filePath) {
    nlohmann::json j;
    auto& eventsJson = j["Stage1_LevelData"]["Events"];
    eventsJson = nlohmann::json::array();

    for (const auto& ev : allEvents_) {
        nlohmann::json eventJson = ev.parameters;
        eventJson["TriggerDistance"] = ev.triggerDistance;
        eventJson["Type"] = ev.eventType;
        eventsJson.push_back(eventJson);
    }

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << j.dump(4);
    } else {
        Log::OutPutLog(std::cout, "[WaveManager] Failed to save level data: " + filePath + "\n");
    }
}
