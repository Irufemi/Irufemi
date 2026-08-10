#include "WaveEventHandlers.h"
#include "DebugEnemySpawnerComponent.h"
#include "Framework/BaseScene.h"
#include "Framework/GameObject.h"
#include "Renderer/System/Core/BaseModel.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/SceneManager.h"
#include "Resource/Audio/AudioManager.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>

void SpawnEnemyHandler::Execute(const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    // パラメータからオフセットを取得
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    
    if (data.parameters.contains("OffsetFromRail")) {
        auto offset = data.parameters["OffsetFromRail"];
        if (offset.contains("x")) offsetX = offset["x"].get<float>();
        if (offset.contains("y")) offsetY = offset["y"].get<float>();
        if (offset.contains("z")) offsetZ = offset["z"].get<float>();
    }

    // レール座標を基準にオフセットをワールド座標へ変換
    // Zはレール進行方向、Xはレール右方向、Yは上方向（ここでは簡易的にYを絶対上方向とする）
    Irufemi::Vector3 spawnPos = railPos;
    spawnPos.x += railRight.x * offsetX + railForward.x * offsetZ;
    spawnPos.y += offsetY; // 簡易的に絶対Y
    spawnPos.z += railRight.z * offsetX + railForward.z * offsetZ;

    Irufemi::Vector3 spawnRot = {0.0f, std::atan2(-railForward.x, -railForward.z), 0.0f}; // プレイヤー方向を向く

    // DebugEnemySpawner を探してスポーンを依頼する
    int count = 1;
    std::string formation = "Center";
    if (data.parameters.contains("Count")) {
        count = data.parameters["Count"].get<int>();
    }
    if (data.parameters.contains("Formation")) {
        formation = data.parameters["Formation"].get<std::string>();
    }

    auto engine = BaseModel::GetIrufemiEngine();
    auto scene = engine ? engine->GetSceneManager()->GetCurrentScene() : nullptr;
    if (auto baseScene = dynamic_cast<BaseScene*>(scene)) {
        auto spawnerObj = baseScene->FindGameObject("EnemySpawner"); 
        if (spawnerObj) {
            if (auto spawner = spawnerObj->GetComponent<DebugEnemySpawnerComponent>()) {
                for (int i = 0; i < count; ++i) {
                    Irufemi::Vector3 currentSpawnPos = spawnPos;
                    // 簡単なフォーメーションの計算
                    if (formation == "V_Shape" && count > 1) {
                        // 先頭を0番目とし、以降を左右に広げる
                        if (i > 0) {
                            float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
                            float distanceBack = 5.0f * ((i + 1) / 2);
                            float distanceSide = 5.0f * ((i + 1) / 2) * sideSign;
                            currentSpawnPos.x += railRight.x * distanceSide - railForward.x * distanceBack;
                            currentSpawnPos.y += railRight.y * distanceSide - railForward.y * distanceBack;
                            currentSpawnPos.z += railRight.z * distanceSide - railForward.z * distanceBack;
                        }
                    } else if (formation == "Line" && count > 1) {
                        // 横一列
                        float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
                        float distanceSide = 5.0f * ((i + 1) / 2) * sideSign;
                        currentSpawnPos.x += railRight.x * distanceSide;
                        currentSpawnPos.y += railRight.y * distanceSide;
                        currentSpawnPos.z += railRight.z * distanceSide;
                    }
                    spawner->SpawnEnemy(currentSpawnPos, spawnRot);
                }
                Log::OutPutLog(std::cout, "[WaveManager] Spawned " + std::to_string(count) + " enemies (Formation: " + formation + ") at distance: " + std::to_string(data.triggerDistance) + "\n");
                return;
            }
        }
    }
    
    Log::OutPutLog(std::cout, "[WaveManager] Warning: DebugEnemySpawner not found.\n");
}

void PlayBGMHandler::Execute(const WaveEventData& data, const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
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
