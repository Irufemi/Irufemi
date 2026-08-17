#include "Level/WaveEventHandlers.h"
#include "Combat/DebugEnemySpawnerComponent.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/GameObject/GameObject.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Scene/SceneManager.h"
#include "Audio/AudioManager.h"
#include "Core/Utility/Log.h"
#include "Level/WaveManagerComponent.h"
#include "Framework/Component/Logic/SpawnPointComponent.h"
#include "Framework/Component/TransformComponent.h"
#include <iostream>

std::vector<Irufemi::Vector3> SpawnEnemyHandler::CalculateSpawnPositions(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    std::vector<Irufemi::Vector3> positions;

    // 1. GroupId / WaveId から SpawnPoint を取得
    std::string waveId = "Unknown";
    if (data.parameters.contains("WaveId")) {
        waveId = data.parameters["WaveId"].get<std::string>();
    } else if (data.parameters.contains("GroupId")) {
        waveId = data.parameters["GroupId"].get<std::string>();
    }

    Irufemi::Vector3 spawnPos = railPos;

    if (manager) {
        const auto& spawnPoints = manager->GetSpawnPoints(waveId);
        if (!spawnPoints.empty()) {
            // 見つかった最初のSpawnPointを基準座標として使用する
            if (auto t = spawnPoints[0]->GetGameObject()->GetComponent<TransformComponent>()) {
                spawnPos = t->GetPosition();
            }
        } else if (waveId != "Unknown") {
            Log::OutPutLog(std::cout, "[WaveManager] Warning: SpawnPoint with WaveId '" + waveId + "' not found in preview/execute.\n");
        }
    }

    int count = 1;
    std::string formation = "Center";
    if (data.parameters.contains("Count")) count = data.parameters["Count"].get<int>();
    if (data.parameters.contains("Formation")) formation = data.parameters["Formation"].get<std::string>();

    for (int i = 0; i < count; ++i) {
        Irufemi::Vector3 currentSpawnPos = spawnPos;
        // 簡単なフォーメーションの計算
        if (formation == "V_Shape" && count > 1) {
            if (i > 0) {
                float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
                float distanceBack = 5.0f * ((i + 1) / 2);
                float distanceSide = 5.0f * ((i + 1) / 2) * sideSign;
                currentSpawnPos.x += railRight.x * distanceSide - railForward.x * distanceBack;
                currentSpawnPos.y += railRight.y * distanceSide - railForward.y * distanceBack;
                currentSpawnPos.z += railRight.z * distanceSide - railForward.z * distanceBack;
            }
        } else if (formation == "Line" && count > 1) {
            float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
            float distanceSide = 5.0f * ((i + 1) / 2) * sideSign;
            currentSpawnPos.x += railRight.x * distanceSide;
            currentSpawnPos.y += railRight.y * distanceSide;
            currentSpawnPos.z += railRight.z * distanceSide;
        }
        positions.push_back(currentSpawnPos);
    }

    return positions;
}

void SpawnEnemyHandler::Execute(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    auto positions = CalculateSpawnPositions(manager, data, railPos, railForward, railRight);
    Irufemi::Vector3 spawnRot = {0.0f, std::atan2(-railForward.x, -railForward.z), 0.0f};

    auto engine = BaseModel::GetIrufemiEngine();
    auto scene = engine ? engine->GetSceneManager()->GetCurrentScene() : nullptr;
    if (auto baseScene = dynamic_cast<BaseScene*>(scene)) {
        auto spawnerObj = baseScene->FindGameObject("EnemySpawner"); 
        if (spawnerObj) {
            if (auto spawner = spawnerObj->GetComponent<DebugEnemySpawnerComponent>()) {
                for (const auto& pos : positions) {
                    spawner->SpawnEnemy(pos, spawnRot);
                }
                Log::OutPutLog(std::cout, "[WaveManager] Spawned " + std::to_string(positions.size()) + " enemies at distance: " + std::to_string(data.triggerDistance) + "\n");
                return;
            }
        }
    }
    
    Log::OutPutLog(std::cout, "[WaveManager] Warning: DebugEnemySpawner not found.\n");
}

#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"

void SpawnEnemyHandler::DrawEditorPreview(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    auto positions = CalculateSpawnPositions(manager, data, railPos, railForward, railRight);
    
    auto engine = BaseModel::GetIrufemiEngine();
    if (engine && engine->GetDebugPrimitiveRenderer()) {
        Irufemi::Vector4 color = {1.0f, 0.0f, 0.0f, 1.0f}; // 赤色のキューブ
        
        for (const auto& pos : positions) {
            float scale = 2.0f;
            Irufemi::Matrix4x4 transform = {
                scale, 0, 0, 0,
                0, scale, 0, 0,
                0, 0, scale, 0,
                pos.x, pos.y + (scale * 0.5f), pos.z, 1.0f // 地面にめり込まないように少し上げる
            };
            engine->GetDebugPrimitiveRenderer()->AddCube(transform, color);
        }
    }
}
#endif

void PlayBGMHandler::Execute(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    std::string track = "Unknown";
    if (data.parameters.contains("Track")) {
        track = data.parameters["Track"].get<std::string>();
    }
    Log::OutPutLog(std::cout, "[WaveManager] Playing BGM: " + track + " at distance: " + std::to_string(data.triggerDistance) + "\n");

    auto engine = BaseModel::GetIrufemiEngine();
    if (engine) {
        auto audioManager = engine->GetAudioManager();
        if (audioManager) {
            // トラック名（例："Stage1_Theme"）からサウンドデータを取得してループ再生
            auto soundData = audioManager->GetOrLoadSoundByFile("resources/audio/" + track + ".wav", track);
            if (soundData) {
                audioManager->Play(soundData, true, 1.0f);
            } else {
                Log::OutPutLog(std::cout, "[WaveManager] BGM file not found: resources/audio/" + track + ".wav\n");
            }
        }
    }
}
