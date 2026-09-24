#include "Level/WaveEventHandlers.h"
#include "Combat/EnemySpawnerComponent.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/GameObject/GameObject.h"
#include "Core/System/IrufemiEngine.h"
#include "Framework/Scene/SceneManager.h"
#include "Audio/AudioManager.h"
#include "Core/Utility/Log.h"
#include "Level/WaveManagerComponent.h"
#include "Framework/Component/Logic/SpawnPointComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Renderer/Object/3D/StaticModelObject/StaticModelObject.h"
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include "RailMechanics/RailShooterEnemyComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Core/Math/MathFunction.h"
#include <iostream>

std::vector<Irufemi::Vector3> SpawnEnemyHandler::CalculateSpawnPositions(WaveManagerComponent* manager,
                                                                         const WaveEventData& data,
                                                                         const Irufemi::Vector3& railPos,
                                                                         const Irufemi::Vector3& railForward,
                                                                         const Irufemi::Vector3& railRight) {
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
            Log::OutPutLog(std::cout, "[WaveManager] Warning: SpawnPoint with WaveId '" + waveId +
                                          "' not found in preview/execute.\n");
        }
    }

    // 2. OffsetFromRail の加算 (レール基準座標系: X = railRight, Y = 上方向(0,1,0), Z = railForward)
    if (data.parameters.contains("OffsetFromRail")) {
        const auto& offsetJson = data.parameters["OffsetFromRail"];
        float ox = offsetJson.value("x", 0.0f);
        float oy = offsetJson.value("y", 0.0f);
        float oz = offsetJson.value("z", 0.0f);

        Irufemi::Vector3 railUp = {0.0f, 1.0f, 0.0f};
        spawnPos.x += railRight.x * ox + railUp.x * oy + railForward.x * oz;
        spawnPos.y += railRight.y * ox + railUp.y * oy + railForward.y * oz;
        spawnPos.z += railRight.z * ox + railUp.z * oy + railForward.z * oz;
    }

    int count = 1;
    std::string formation = "Center";
    if (data.parameters.contains("Count")) {
        count = data.parameters["Count"].get<int>();
    }
    if (data.parameters.contains("Formation")) {
        formation = data.parameters["Formation"].get<std::string>();
    }
    float formationSpacing = data.parameters.value("FormationSpacing", 5.0f);

    for (int i = 0; i < count; ++i) {
        Irufemi::Vector3 currentSpawnPos = spawnPos;
        // 簡単なフォーメーションの計算
        if (formation == "V_Shape" && count > 1) {
            if (i > 0) {
                float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
                float distanceBack = formationSpacing * ((i + 1) / 2);
                float distanceSide = formationSpacing * ((i + 1) / 2) * sideSign;
                currentSpawnPos.x += railRight.x * distanceSide - railForward.x * distanceBack;
                currentSpawnPos.y += railRight.y * distanceSide - railForward.y * distanceBack;
                currentSpawnPos.z += railRight.z * distanceSide - railForward.z * distanceBack;
            }
        } else if (formation == "Line" && count > 1) {
            float sideSign = (i % 2 == 0) ? 1.0f : -1.0f;
            float distanceSide = formationSpacing * ((i + 1) / 2) * sideSign;
            currentSpawnPos.x += railRight.x * distanceSide;
            currentSpawnPos.y += railRight.y * distanceSide;
            currentSpawnPos.z += railRight.z * distanceSide;
        }
        positions.push_back(currentSpawnPos);
    }

    return positions;
}

EnemySpawnerComponent* SpawnEnemyHandler::GetOrFindSpawner(WaveManagerComponent* manager) {
    if (cachedSpawnerComp_ && !cachedSpawnerObj_.expired()) {
        return cachedSpawnerComp_;
    }

    cachedSpawnerComp_ = nullptr;
    cachedSpawnerObj_.reset();

    BaseScene* baseScene = nullptr;
    if (manager && manager->GetGameObject()) {
        baseScene = dynamic_cast<BaseScene*>(manager->GetGameObject()->GetScene());
    }
    if (!baseScene && manager) {
        if (auto engine = manager->GetEngine()) {
            if (auto sceneManager = engine->GetSceneManager()) {
                baseScene = dynamic_cast<BaseScene*>(sceneManager->GetCurrentScene());
            }
        }
    }

    if (baseScene) {
        if (auto spawnerObj = baseScene->FindGameObject("EnemySpawner")) {
            cachedSpawnerObj_ = spawnerObj;
            cachedSpawnerComp_ = spawnerObj->GetComponent<EnemySpawnerComponent>();
        }
    }

    return cachedSpawnerComp_;
}

void SpawnEnemyHandler::Execute(WaveManagerComponent* manager, const WaveEventData& data,
                                const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward,
                                const Irufemi::Vector3& railRight) {
    auto positions = CalculateSpawnPositions(manager, data, railPos, railForward, railRight);
    Irufemi::Vector3 spawnRot = {0.0f, std::atan2(-railForward.x, -railForward.z), 0.0f};

    float combatDuration = data.parameters.value("CombatDuration", 7.5f);
    float targetDistance = data.parameters.value("TargetDistance", 65.0f);
    float scaleMultiplier = data.parameters.value("Scale", 1.0f);

    if (auto spawner = GetOrFindSpawner(manager)) {
        for (const auto& pos : positions) {
            if (auto enemyObj = spawner->SpawnEnemy(pos, spawnRot, scaleMultiplier)) {
                if (auto enemyComp = enemyObj->GetComponent<RailShooterEnemyComponent>()) {
                    enemyComp->SetCombatDuration(combatDuration);
                    enemyComp->SetTargetDistance(targetDistance);
                }
            }
        }
        Log::OutPutLog(std::cout, "[WaveManager] Spawned " + std::to_string(positions.size()) +
                                      " enemies at distance: " + std::to_string(data.triggerDistance) + "\n");
        return;
    }

    Log::OutPutLog(std::cout, "[WaveManager] Warning: EnemySpawner not found.\n");
}

#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "Renderer/Object/Batch/DebugPrimitiveRenderer.h"
#include "Framework/Prefab/PrefabUtility.h"

void SpawnEnemyHandler::DrawEditorPreview(WaveManagerComponent* manager, const WaveEventData& data,
                                          const Irufemi::Vector3& railPos, const Irufemi::Vector3& railForward,
                                          const Irufemi::Vector3& railRight) {
    auto positions = CalculateSpawnPositions(manager, data, railPos, railForward, railRight);

    auto engine = manager ? manager->GetEngine() : nullptr;
    if (!engine) {
        return;
    }

    std::string modelPath = "Enemy_GravityGolem_A/SM_Enemy_GravityGolem_A.obj";
    Irufemi::Vector3 baseScale = {1.2f, 1.2f, 1.2f};
    float baseRadius = 2.0f;

    if (auto spawner = GetOrFindSpawner(manager)) {
        modelPath = spawner->GetEnemyModelPath();
        baseScale = spawner->GetBaseEnemyScale();
        baseRadius = spawner->GetBaseColliderRadius();
    } else {
        // スポナーが見つからない場合もプレハブから自動解決
        auto metrics = PrefabUtility::ExtractMetrics("resources/prefabs/Enemy_GravityGolem.json");
        if (!metrics.modelPath.empty()) {
            modelPath = metrics.modelPath;
        }
        baseScale = metrics.baseScale;
        if (metrics.hasSphereCollider) {
            baseRadius = metrics.colliderRadius;
        }
    }

    float scaleMultiplier = data.parameters.value("Scale", 1.0f);
    Irufemi::Vector3 finalScale = baseScale * scaleMultiplier;
    float finalRadius = baseRadius * scaleMultiplier;

    auto previewBatch = manager->GetPreviewBatchRenderer(modelPath);
    if (previewBatch) {
        for (size_t i = 0; i < positions.size(); ++i) {
            Irufemi::Vector3 rot = {0.0f, std::atan2(-railForward.x, -railForward.z), 0.0f};
            Irufemi::Matrix4x4 transform = Irufemi::Math::MakeAffineMatrix(finalScale, rot, positions[i]);
            previewBatch->AddInstanceWorld(transform);

            // 当たり判定の球体（ワイヤー球: エメラルドグリーン）をリアルタイム描画
            if (auto debugRenderer = engine->GetDebugPrimitiveRenderer()) {
                Irufemi::Vector4 sphereColor = {0.0f, 1.0f, 0.5f, 0.85f}; // 見やすい緑色のワイヤー球
                debugRenderer->AddSphere(positions[i], finalRadius, sphereColor, DebugCategory::Level);
            }
        }
    } else if (auto debugRenderer = engine->GetDebugPrimitiveRenderer()) {
        Irufemi::Vector4 sphereColor = {0.0f, 1.0f, 0.5f, 0.85f};
        for (const auto& pos : positions) {
            debugRenderer->AddSphere(pos, finalRadius, sphereColor, DebugCategory::Level);
        }
    }
}
#endif

void PlayBGMHandler::Execute(WaveManagerComponent* manager, const WaveEventData& data, const Irufemi::Vector3& railPos,
                             const Irufemi::Vector3& railForward, const Irufemi::Vector3& railRight) {
    std::string track = "Unknown";
    if (data.parameters.contains("Track")) {
        track = data.parameters["Track"].get<std::string>();
    }
    Log::OutPutLog(std::cout, "[WaveManager] Playing BGM: " + track +
                                  " at distance: " + std::to_string(data.triggerDistance) + "\n");

    auto engine = manager ? manager->GetEngine() : nullptr;
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
