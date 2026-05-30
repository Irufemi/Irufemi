#include "building.h"
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <format>
#include <Windows.h>
#include <cmath>

#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "Renderer/VoxelParticle/VoxelParticleManager.h"
#include "Renderer/Particle/ParticleSystem.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Renderer/Region/ModelRegion.h"
#include "Engine/IrufemiEngine.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

using json = nlohmann::json;

Building::Building() {}

Building::~Building() {}

void Building::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    buildingRegion_ = std::make_unique<ModelRegion>();
    buildingRegion_->Initialize("building/building.obj");

    LoadJson();

    // VoxelParticleManager にプールの事前確保を依頼
    int poolSize = params_.maxCount + 10;
    engine_->GetVoxelParticleManager()->ReservePool("building/block.obj", {32, 32, 32}, poolSize);

    // 砂煙エフェクトの初期化
    spawnDustSystem_ = std::make_unique<ParticleSystem>();
    // PrimitiveType::Plane を指定
    spawnDustSystem_->Initialize("resources/circle.png", ParticleType::kBuildingSpawnDust, PrimitiveType::Plane);
    spawnDustSystem_->SetCullingEnabled(false); // 複数箇所で共有するためカリングは無効化

    Generate();

#ifdef USE_IMGUI
    if (engine_) {
        debugLines_ = std::make_unique<Line3DRegion>();
        debugLines_->Initialize();
    }
#endif
}



void Building::Update() {
    for (auto it = instances_.begin(); it != instances_.end(); ) {
        auto& inst = *it;

        // 完全消滅済みならスキップ
        if (inst.isDestroyed) {
            it = instances_.erase(it);
            continue;
        }

        // 出現演出中
        if (inst.isSpawning) {
            inst.spawnTimer += 1.0f / 60.0f;
            float t = inst.spawnTimer / inst.spawnDuration;

            // 砂煙エフェクトの発生（上昇中のみ）
            if (spawnDustSystem_ && t < 1.0f) {
                // inst.scale.x * 2.0f が実際のビルの幅
                spawnDustSystem_->SetEmitterArea({inst.scale.x * 2.0f, 0.0f, inst.scale.z * 2.0f});
                
                // 【最適化】毎フレーム30個生成すると、ビル複数出現時にCPU負荷が跳ね上がりフレーム落ちの原因になるため、
                // 0.05秒に1回、5個のパーティクルを放出するようにして負荷を劇的に下げる
                int prevFrame = static_cast<int>((inst.spawnTimer - 1.0f / 60.0f) / 0.05f);
                int currFrame = static_cast<int>(inst.spawnTimer / 0.05f);
                if (prevFrame != currFrame) {
                    spawnDustSystem_->PlayHitEffect({inst.position.x, 0.0f, inst.position.z}, 5);
                }
            }

            if (t >= 1.0f) {
                t = 1.0f;
                inst.isSpawning = false;
                inst.position = inst.targetPosition;
            } else {
                // 上昇イージング (OutQuad)
                float easeT = t * (2.0f - t);
                float currentY = inst.initialY + (inst.targetPosition.y - inst.initialY) * easeT;

                // 地響きのような減衰する左右の揺れ
                float shakeAmp = inst.scale.x * 0.08f;
                float shakeFactor = 1.0f - t; 
                float shakeX = std::sin(inst.spawnTimer * 45.0f) * shakeAmp * shakeFactor;
                float shakeZ = std::cos(inst.spawnTimer * 53.0f) * shakeAmp * shakeFactor;

                inst.position = {
                    inst.targetPosition.x + shakeX,
                    currentY,
                    inst.targetPosition.z + shakeZ
                };
            }


            ++it;
            continue;
        }

        if (inst.isBlownAway) {
            // 重力を適用して放物線を描くようにする
            inst.blowVelocity.y -= params_.blowGravity;

            // 吹き飛び中の移動
            inst.position = Math::Add(inst.position, inst.blowVelocity);
            inst.rotate.x += inst.angularVelocity.x;
            inst.rotate.y += inst.angularVelocity.y;
            inst.rotate.z += inst.angularVelocity.z;

            // 地面バウンド処理
            float floorY = inst.scale.x * params_.floorHeightRatio * inst.floorCount * 0.5f;
            if (inst.position.y < floorY) {
                inst.position.y = floorY;
                inst.blowVelocity.y *= params_.blowBounceY; // 反発係数
                inst.blowVelocity.x *= params_.blowFrictionXZ;  // 摩擦（弱め）
                inst.blowVelocity.z *= params_.blowFrictionXZ;
                inst.angularVelocity.x *= params_.blowAngularFriction;
                inst.angularVelocity.y *= params_.blowAngularFriction;
                inst.angularVelocity.z *= params_.blowAngularFriction;
            }

            // 壁反射
            const float bound = BuildingInstance::kFieldBound;
            const float r = inst.scale.x;
            if (inst.position.x - r < -bound) {
                inst.position.x = -bound + r;
                inst.blowVelocity.x *= -1.0f;
            } else if (inst.position.x + r > bound) {
                inst.position.x = bound - r;
                inst.blowVelocity.x *= -1.0f;
            }
            if (inst.position.z - r < -bound) {
                inst.position.z = -bound + r;
                inst.blowVelocity.z *= -1.0f;
            } else if (inst.position.z + r > bound) {
                inst.position.z = bound - r;
                inst.blowVelocity.z *= -1.0f;
            }

            // 消滅タイマー
            float prevTimer = inst.disappearTimer;
            inst.disappearTimer += 1.0f / 60.0f;

            // 停止判定: 速度と角速度が一定値以下なら即爆発
            float speedSq = inst.blowVelocity.x * inst.blowVelocity.x + inst.blowVelocity.y * inst.blowVelocity.y + inst.blowVelocity.z * inst.blowVelocity.z;
            float angSq = inst.angularVelocity.x * inst.angularVelocity.x + inst.angularVelocity.y * inst.angularVelocity.y + inst.angularVelocity.z * inst.angularVelocity.z;
            if (!inst.hasExploded && speedSq < 0.01f && angSq < 0.001f) {
                inst.disappearTimer = BuildingInstance::kDisappearTime;
            }

            // ボクセル爆散トリガー
            if (!inst.hasExploded && prevTimer < BuildingInstance::kDisappearTime &&
                inst.disappearTimer >= BuildingInstance::kDisappearTime) {
                // building.obj と block.obj のXZサイズの差異を補正するため、XZスケールを2倍にして爆破する
                Vector3 explodeScale = { inst.scale.x * 2.0f, inst.scale.x * params_.floorHeightRatio * inst.floorCount, inst.scale.z * 2.0f };
                
                /// プレイヤー攻撃後の寿命崩壊は、青白く四散する DebrisExplosive を指定
                engine_->GetVoxelParticleManager()->PlayExplosion("building/block.obj", inst.position, inst.blowVelocity, inst.rotate, explodeScale, VoxelParticleSystem::ParticleType::DebrisExplosive);
                inst.hasExploded = true;
            }

            // モデル消滅判定
            if (inst.disappearTimer >= BuildingInstance::kDisappearTime) {
                inst.isDestroyed = true;
            }
        }



        ++it;
    }

#ifdef USE_IMGUI
    if (engine_ && engine_->GetInputManager()->IsKeyPressedDIK(0x3B /*DIK_F1*/)) {
        isDebugDraw_ = !isDebugDraw_;
    }

    if (debugLines_) {
        debugLines_->ClearInstances();
        if (isDebugDraw_) {
            Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green

            for (auto& inst : instances_) {
                bool modelGone = inst.isBlownAway && inst.disappearTimer >= BuildingInstance::kDisappearTime;
                if (inst.isDestroyed || modelGone) continue;

                OBB obb = GetBuildingOBB(static_cast<int>(&inst - &instances_[0]));

                // OBBの8頂点を計算して辺を描画
                Vector3 corners[8];
                for (int i = 0; i < 8; ++i) {
                    Vector3 offset = { 0, 0, 0 };
                    offset = Math::Add(offset, Math::Multiply((i & 1) ? obb.size.x : -obb.size.x, obb.orientations[0]));
                    offset = Math::Add(offset, Math::Multiply((i & 2) ? obb.size.y : -obb.size.y, obb.orientations[1]));
                    offset = Math::Add(offset, Math::Multiply((i & 4) ? obb.size.z : -obb.size.z, obb.orientations[2]));
                    corners[i] = Math::Add(obb.center, offset);
                }
                // 底面
                debugLines_->AddInstance(corners[0], corners[1], color);
                debugLines_->AddInstance(corners[1], corners[3], color);
                debugLines_->AddInstance(corners[3], corners[2], color);
                debugLines_->AddInstance(corners[2], corners[0], color);
                // 上面
                debugLines_->AddInstance(corners[4], corners[5], color);
                debugLines_->AddInstance(corners[5], corners[7], color);
                debugLines_->AddInstance(corners[7], corners[6], color);
                debugLines_->AddInstance(corners[6], corners[4], color);
                // 垂直
                debugLines_->AddInstance(corners[0], corners[4], color);
                debugLines_->AddInstance(corners[1], corners[5], color);
                debugLines_->AddInstance(corners[2], corners[6], color);
                debugLines_->AddInstance(corners[3], corners[7], color);
            }
        }
        debugLines_->Update();
    }
#endif

    // 全てのビルから集まった砂煙パーティクルを一括更新
    if (spawnDustSystem_) {
        spawnDustSystem_->Update();
    }
}

void Building::Draw(IrufemiEngine* engine) {
    if (buildingRegion_) {
        buildingRegion_->ClearInstances();
    }

    for (auto& inst : instances_) {
        // scale.yをfloorCount*floorHeightで常に同期（ImGui変更に追従）
        // floorHeightはscaleXZに比例
        float floorHeight = inst.scale.x * params_.floorHeightRatio;
        inst.scale.y = inst.floorCount * floorHeight;

        // 完全消滅済みなら描画しない
        if (inst.isDestroyed) {
            continue;
        }

        // モデル描画（消滅タイマー前のみ）
        bool modelGone = inst.isBlownAway && inst.disappearTimer >= BuildingInstance::kDisappearTime;
        if (!modelGone) {
            // ビル全体の高さ
            float fh = inst.scale.x * params_.floorHeightRatio;
            float totalHeight = inst.floorCount * fh;
            // 回転行列（全階層共通）
            Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(inst.rotate);

            for (int floor = 0; floor < inst.floorCount; ++floor) {
                // ローカル空間での各階層の中心Y座標（ビル中心を原点とする）
                float localY = -totalHeight / 2.0f + fh * floor + fh / 2.0f;

                // ローカルオフセットを回転行列で変換
                Vector3 localOffset = { 0.0f, localY, 0.0f };
                Vector3 rotatedOffset = {
                    rotMat.m[0][0] * localOffset.x + rotMat.m[0][1] * localOffset.y + rotMat.m[0][2] * localOffset.z,
                    rotMat.m[1][0] * localOffset.x + rotMat.m[1][1] * localOffset.y + rotMat.m[1][2] * localOffset.z,
                    rotMat.m[2][0] * localOffset.x + rotMat.m[2][1] * localOffset.y + rotMat.m[2][2] * localOffset.z
                };

                Transform tf;
                tf.translate = Math::Add(inst.position, rotatedOffset);
                tf.rotate = inst.rotate;
                tf.scale = { inst.scale.x, fh, inst.scale.z };
                if (buildingRegion_) {
                    buildingRegion_->AddInstance(tf, {1.0f, 1.0f, 1.0f, 1.0f});
                }
            }
        }


    }

    if (buildingRegion_) {
        buildingRegion_->Draw();
    }

    // 砂煙エフェクトの一括描画
    if (spawnDustSystem_) {
        spawnDustSystem_->Draw();
    }

    // ループ後に標準PSOを復元（後続の描画に影響しないように）
    engine->SetBlend(BlendMode::kBlendModeNormal);
    engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
    engine->SetCull(PSOManager::CullMode::Back);

#ifdef USE_IMGUI
    if (debugLines_ && isDebugDraw_ && engine_) {
        debugLines_->Draw();
    }
#endif
}

void Building::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::Begin("ビル設定")) {
        bool changed = false;

        changed |= ImGui::SliderInt("初期配置されるビルの個数", &params_.count, 1, 100);
        changed |= ImGui::DragFloat("ビルが生成されるフィールドのXZ範囲(中心からの距離)", &params_.fieldRange, 1.0f, 10.0f, 200.0f);
        changed |= ImGui::DragInt("ビルの最小階層数", &params_.minFloors, 1, 1, 100);
        changed |= ImGui::DragInt("ビルの最大階層数", &params_.maxFloors, 1, 1, 100);
        changed |= ImGui::DragFloat("Floor Height Ratio", &params_.floorHeightRatio, 0.01f, 0.1f, 2.0f);
        changed |= ImGui::DragFloat("ビルの最小幅スケール(XZ)", &params_.minScaleXZ, 0.1f, 0.1f, 50.0f);
        changed |= ImGui::DragFloat("ビルの最大幅スケール(XZ)", &params_.maxScaleXZ, 0.1f, 0.1f, 50.0f);
        changed |= ImGui::DragFloat("ビル同士が生成時に最低限離れるべき距離", &params_.minDistance, 0.1f, 0.0f, 100.0f);
        changed |= ImGui::DragInt("ビルの初期体力値", &params_.buildingHp, 1, 1, 10000);

        ImGui::Separator();
        ImGui::Text("自動スポーン設定");
        changed |= ImGui::DragFloat("ビルが自動生成される時間間隔(秒)", &params_.spawnInterval, 0.1f, 0.1f, 60.0f);
        changed |= ImGui::SliderInt("時間経過で自動生成されるビルの最大上限数", &params_.maxCount, 1, 100);
        changed |= ImGui::DragFloat("プレイヤーの現在座標を避ける半径", &params_.avoidPlayerRadius, 0.5f, 0.0f, 100.0f);
        changed |= ImGui::DragFloat("ボスが移動/攻撃対象にしている座標を避ける半径", &params_.avoidBossRadius, 0.5f, 0.0f, 100.0f);
        changed |= ImGui::DragFloat("ビルが地面からせり上がる速度(秒速)", &params_.spawnSpeed, 0.1f, 0.1f, 100.0f);

        ImGui::Separator();
        ImGui::Text("吹き飛び物理設定");
        changed |= ImGui::DragFloat("重力(Blow Gravity)", &params_.blowGravity, 0.001f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("バウンド係数(Blow Bounce Y)", &params_.blowBounceY, 0.01f, -1.0f, 0.0f);
        changed |= ImGui::DragFloat("摩擦(Blow Friction XZ)", &params_.blowFrictionXZ, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("回転摩擦(Blow Angular Friction)", &params_.blowAngularFriction, 0.01f, 0.0f, 1.0f);
        changed |= ImGui::DragFloat("拡散力(Scatter Spread)", &params_.scatterSpread, 0.01f, 0.0f, 5.0f);
        changed |= ImGui::DragFloat("上方向力ベース(Up Force Base)", &params_.scatterUpForceBase, 0.01f, 0.0f, 5.0f);
        changed |= ImGui::DragFloat("上方向力ランダム(Up Force Rand)", &params_.scatterUpForceRand, 0.01f, 0.0f, 5.0f);
        changed |= ImGui::DragFloat("速度ベース(Speed Base)", &params_.scatterSpeedBase, 0.01f, 0.0f, 5.0f);
        changed |= ImGui::DragFloat("速度ランダム(Speed Rand)", &params_.scatterSpeedRand, 0.01f, 0.0f, 5.0f);
        changed |= ImGui::DragFloat("回転速度(Angular Velocity)", &params_.scatterAngularVelocity, 0.01f, 0.0f, 5.0f);

        // 最小値と最大値の整合性を保つ
        if (params_.minFloors > params_.maxFloors) {
            params_.maxFloors = params_.minFloors;
        }
        if (params_.minScaleXZ > params_.maxScaleXZ) {
            params_.maxScaleXZ = params_.minScaleXZ;
        }

        if (ImGui::Button("Regenerate")) {
            Generate();
        }

        ImGui::SameLine();
        
        if (ImGui::Button("Save")) {
            SaveJson();
        }

        ImGui::Separator();

        // 残存建物数
        int alive = GetAliveBuildingCount();
        ImGui::Text("Active Buildings: %d / %d", alive, (int)instances_.size());

    }
    ImGui::End();
#endif
}

void Building::LoadJson() {
    std::ifstream file(kJsonFilePath);
    if (!file.is_open()) {
        OutputDebugStringA("Building: Failed to load JSON (file not found).\n");
        return;
    }

    json j;
    try {
        file >> j;
    } catch (json::parse_error&) {
        return;
    }

    if (j.contains("building")) {
        const auto& b = j["building"];
        if (b.contains("count")) params_.count = b["count"];
        if (b.contains("min_floors")) params_.minFloors = b["min_floors"];
        if (b.contains("max_floors")) params_.maxFloors = b["max_floors"];
        if (b.contains("floor_height_ratio")) params_.floorHeightRatio = b["floor_height_ratio"];
        if (b.contains("min_scale_xz")) params_.minScaleXZ = b["min_scale_xz"];
        if (b.contains("max_scale_xz")) params_.maxScaleXZ = b["max_scale_xz"];
        if (b.contains("field_range")) params_.fieldRange = b["field_range"];
        if (b.contains("min_distance")) params_.minDistance = b["min_distance"];
        if (b.contains("building_hp")) params_.buildingHp = b["building_hp"];
        if (b.contains("spawn_interval")) params_.spawnInterval = b["spawn_interval"];
        if (b.contains("max_count")) params_.maxCount = b["max_count"];
        if (b.contains("avoid_player_radius")) params_.avoidPlayerRadius = b["avoid_player_radius"];
        if (b.contains("avoid_boss_radius")) params_.avoidBossRadius = b["avoid_boss_radius"];
        if (b.contains("spawn_speed")) params_.spawnSpeed = b["spawn_speed"];
        if (b.contains("blow_gravity")) params_.blowGravity = b["blow_gravity"];
        if (b.contains("blow_bounce_y")) params_.blowBounceY = b["blow_bounce_y"];
        if (b.contains("blow_friction_xz")) params_.blowFrictionXZ = b["blow_friction_xz"];
        if (b.contains("blow_angular_friction")) params_.blowAngularFriction = b["blow_angular_friction"];
        if (b.contains("scatter_spread")) params_.scatterSpread = b["scatter_spread"];
        if (b.contains("scatter_up_force_base")) params_.scatterUpForceBase = b["scatter_up_force_base"];
        if (b.contains("scatter_up_force_rand")) params_.scatterUpForceRand = b["scatter_up_force_rand"];
        if (b.contains("scatter_speed_base")) params_.scatterSpeedBase = b["scatter_speed_base"];
        if (b.contains("scatter_speed_rand")) params_.scatterSpeedRand = b["scatter_speed_rand"];
        if (b.contains("scatter_angular_velocity")) params_.scatterAngularVelocity = b["scatter_angular_velocity"];
        OutputDebugStringA("Building: Parameters loaded from JSON.\n");
    }
}

void Building::SaveJson() {
    json j;
    j["building"] = {
        {"count", params_.count},
        {"count_comment", "初期配置されるビルの個数"},
        {"min_floors", params_.minFloors},
        {"min_floors_comment", "ビルの最小階層数"},
        {"max_floors", params_.maxFloors},
        {"max_floors_comment", "ビルの最大階層数"},
        {"floor_height_ratio", params_.floorHeightRatio},
        {"floor_height_ratio_comment", "1階層の高さ比率 (実際の高さ = scaleXZ * この値)"},
        {"min_scale_xz", params_.minScaleXZ},
        {"min_scale_xz_comment", "ビルの最小幅スケール(XZ)"},
        {"max_scale_xz", params_.maxScaleXZ},
        {"max_scale_xz_comment", "ビルの最大幅スケール(XZ)"},
        {"field_range", params_.fieldRange},
        {"field_range_comment", "ビルが生成されるフィールドのXZ範囲(中心からの距離)"},
        {"min_distance", params_.minDistance},
        {"min_distance_comment", "ビル同士が生成時に最低限離れるべき距離"},
        {"building_hp", params_.buildingHp},
        {"building_hp_comment", "ビルの初期体力値"},
        {"spawn_interval", params_.spawnInterval},
        {"spawn_interval_comment", "ビルが自動生成される時間間隔(秒)"},
        {"max_count", params_.maxCount},
        {"max_count_comment", "時間経過で自動生成されるビルの最大上限数"},
        {"avoid_player_radius", params_.avoidPlayerRadius},
        {"avoid_player_radius_comment", "プレイヤーの現在座標を避ける半径"},
        {"avoid_boss_radius", params_.avoidBossRadius},
        {"avoid_boss_radius_comment", "ボスが移動/攻撃対象にしている座標を避ける半径"},
        {"spawn_speed", params_.spawnSpeed},
        {"spawn_speed_comment", "ビルが地面からせり上がる速度(秒速)"},
        {"blow_gravity", params_.blowGravity},
        {"blow_gravity_comment", "吹き飛び時の重力"},
        {"blow_bounce_y", params_.blowBounceY},
        {"blow_bounce_y_comment", "地面でのバウンド反発係数（負の値）"},
        {"blow_friction_xz", params_.blowFrictionXZ},
        {"blow_friction_xz_comment", "地面での摩擦係数"},
        {"blow_angular_friction", params_.blowAngularFriction},
        {"blow_angular_friction_comment", "地面での回転摩擦係数"},
        {"scatter_spread", params_.scatterSpread},
        {"scatter_spread_comment", "散弾のように飛び散る際のブレ幅"},
        {"scatter_up_force_base", params_.scatterUpForceBase},
        {"scatter_up_force_base_comment", "散弾飛び散り時の上方向への基本力"},
        {"scatter_up_force_rand", params_.scatterUpForceRand},
        {"scatter_up_force_rand_comment", "散弾飛び散り時の上方向への追加ランダム力"},
        {"scatter_speed_base", params_.scatterSpeedBase},
        {"scatter_speed_base_comment", "散弾飛び散り時の基本速度倍率"},
        {"scatter_speed_rand", params_.scatterSpeedRand},
        {"scatter_speed_rand_comment", "散弾飛び散り時の追加ランダム速度倍率"},
        {"scatter_angular_velocity", params_.scatterAngularVelocity},
        {"scatter_angular_velocity_comment", "散弾飛び散り時の基本回転速度"}
    };

    std::ofstream file(kJsonFilePath);
    if (file.is_open()) {
        file << j.dump(4);
        OutputDebugStringA("Building: Parameters saved to JSON.\n");
    }
}

void Building::Generate() {
    OutputDebugStringA("Building: Regenerating buildings...\n");

    // VoxelParticleSystemのUpdate()で登録されたcomputeTasksの中に
    // 破棄予定のインスタンスへのポインタが残っているため、先にクリアする
    if (engine_ && engine_->GetDrawManager()) {
        engine_->GetDrawManager()->ClearAllQueues();
    }


    instances_.clear();

    // パラメータのバリデーション
    int minF = (std::max)(1, params_.minFloors);
    int maxF = (std::max)(minF, params_.maxFloors);
    float floorHRatio = (std::max)(0.1f, params_.floorHeightRatio);

    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<float> distPos(-params_.fieldRange, params_.fieldRange);
    std::uniform_int_distribution<int> distFloors(minF, maxF);
    std::uniform_real_distribution<float> distScaleXZ(params_.minScaleXZ, params_.maxScaleXZ);
    std::uniform_real_distribution<float> distRot(-3.14159265f, 3.14159265f);

    // 配置候補の位置・スケールを先に決めてから ObjClass を生成する
    struct PlacementData {
        Vector3 pos;
        Vector3 scale;
        float rotY;
        int floorCount;
    };
    std::vector<PlacementData> placements;

    for (int i = 0; i < params_.count; ++i) {
        float scaleXZ = 0.0f;
        int floorCount = 0;
        float scaleY = 0.0f;
        Vector3 pos = { 0.0f, 0.0f, 0.0f };
        bool isValidPos = false;
        int maxAttempts = 100;

        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            scaleXZ = distScaleXZ(engine);
            floorCount = distFloors(engine);
            float floorH = scaleXZ * floorHRatio;
            scaleY = floorCount * floorH;
            pos = { distPos(engine), scaleY / 2.0f, distPos(engine) };

            isValidPos = true;
            // building.objはXZ: -1.0~1.0 なので実際の半径 = scaleXZ * √2
            float radiusA = scaleXZ * 1.4142f;

            for (const auto& p : placements) {
                float radiusB = p.scale.x * 1.4142f;
                float dx = pos.x - p.pos.x;
                float dz = pos.z - p.pos.z;
                float distanceSq = dx * dx + dz * dz;
                float requiredDistance = radiusA + radiusB + params_.minDistance;
                if (distanceSq < requiredDistance * requiredDistance) {
                    isValidPos = false;
                    break;
                }
            }

            if (isValidPos) break;
        }

        if (!isValidPos) {
            OutputDebugStringA(std::format("Building: Failed to place building index {}, exceeded max attempts.\n", i).c_str());
            continue;
        }

        placements.push_back({ pos, { scaleXZ, scaleY, scaleXZ }, distRot(engine), floorCount });
    }

    // 実際にインスタンスを生成
    for (auto& pd : placements) {
        BuildingInstance inst;
        inst.position = pd.pos;
        inst.scale = pd.scale;
        inst.rotate = { 0.0f, pd.rotY, 0.0f };
        inst.floorCount = pd.floorCount;
        inst.hp = params_.buildingHp;
        inst.isBlownAway = false;
        inst.isDestroyed = false;
        inst.disappearTimer = 0.0f;
        inst.blowVelocity = {};
        inst.angularVelocity = {};

        instances_.push_back(std::move(inst));
    }



    OutputDebugStringA(std::format("Building: {} buildings generated.\n", (int)instances_.size()).c_str());
}

void Building::ClearAndAddSingleBuilding(const Vector3& position) {

    instances_.clear();

    int minF = (std::max)(1, params_.minFloors);
    int maxF = (std::max)(minF, params_.maxFloors);
    float floorHRatio = (std::max)(0.1f, params_.floorHeightRatio);

    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_int_distribution<int> distFloors(minF, maxF);
    std::uniform_real_distribution<float> distScaleXZ(params_.minScaleXZ, params_.maxScaleXZ);
    std::uniform_real_distribution<float> distRot(-3.14159265f, 3.14159265f);

    float scaleXZ = distScaleXZ(engine);
    int floorCount = distFloors(engine);
    float floorH = scaleXZ * floorHRatio;
    float scaleY = floorCount * floorH;

    BuildingInstance inst;
    inst.scale = { scaleXZ, scaleY, scaleXZ };
    inst.floorCount = floorCount;
    inst.rotate = { 0.0f, distRot(engine), 0.0f };
    inst.hp = params_.buildingHp; 
    inst.isBlownAway = false;
    inst.isDestroyed = false;
    inst.disappearTimer = 0.0f;
    inst.blowVelocity = {0.0f, 0.0f, 0.0f};
    inst.angularVelocity = {0.0f, 0.0f, 0.0f};

    // InGameと同様の出現演出
    inst.isSpawning = true;
    inst.spawnTimer = 0.0f;
    if (params_.spawnSpeed > 0.0f) {
        inst.spawnDuration = scaleY / params_.spawnSpeed;
    } else {
        inst.spawnDuration = 2.0f;
    }
    inst.targetPosition = { position.x, scaleY / 2.0f, position.z };
    inst.initialY = inst.targetPosition.y - scaleY;
    inst.position = { position.x, inst.initialY, position.z };



    instances_.push_back(std::move(inst));
}

void Building::ClearAllBuildings() {

    instances_.clear();
}

// --- 当たり判定用 ---

bool Building::IsBuildingAlive(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    const auto& inst = instances_[index];
    return !inst.isDestroyed && !inst.isBlownAway && !inst.isSpawning && inst.hp > 0;
}

OBB Building::GetBuildingOBB(int index) const {
    OBB obb = {};
    if (index < 0 || index >= static_cast<int>(instances_.size())) return obb;
    const auto& inst = instances_[index];

    bool modelGone = inst.isBlownAway && inst.disappearTimer >= BuildingInstance::kDisappearTime;
    if (modelGone) return obb; // モデル消滅後は判定を消す

    obb.center = inst.position;

    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(inst.rotate);
    obb.orientations[0] = { rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2] };
    obb.orientations[1] = { rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2] };
    obb.orientations[2] = { rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2] };

    // OBBのsizeは中心から面までの距離（half-extent）
    // building.objはXZ: -1.0~1.0（幅2.0）, Y: -0.5~0.5（高さ1.0）
    // XZ half-extent = scale * 1.0, Y half-extent = totalHeight * 0.5
    float totalHeight = inst.floorCount * (inst.scale.x * params_.floorHeightRatio);
    obb.size = { inst.scale.x, totalHeight * 0.5f, inst.scale.z };

    return obb;
}

void Building::ResolvePlayerCollision(Vector3& playerPos, float playerRadius) {
    for (int i = 0; i < static_cast<int>(instances_.size()); ++i) {
        if (!IsBuildingAlive(i)) continue;

        OBB obb = GetBuildingOBB(i);
        Sphere playerSphere;
        playerSphere.center = playerPos;
        playerSphere.radius = playerRadius;

        if (Collision::IsOBBSphereCollision(obb, playerSphere)) {
            // 押し戻し：OBBの中心からプレイヤーへのベクトルで最も浅い軸方向に押し出す
            Vector3 diff = Math::Subtract(playerPos, obb.center);

            // 各軸への射影距離を計算
            float bestPushDist = 1e10f;
            Vector3 bestPushDir = { 0.0f, 0.0f, 0.0f };

            for (int axis = 0; axis < 3; ++axis) {
                if (axis == 1) continue; // Y軸（高さ方向）は押し戻さない

                float proj = Math::Dot(diff, obb.orientations[axis]);
                float overlap = obb.size.x; // axis 0 or 2
                if (axis == 0) overlap = obb.size.x;
                else if (axis == 2) overlap = obb.size.z;

                float penetration = (overlap + playerRadius) - std::abs(proj);
                if (penetration > 0.0f && penetration < bestPushDist) {
                    bestPushDist = penetration;
                    float sign = (proj >= 0.0f) ? 1.0f : -1.0f;
                    bestPushDir = Math::Multiply(sign, obb.orientations[axis]);
                }
            }

            if (bestPushDist < 1e10f) {
                playerPos = Math::Add(playerPos, Math::Multiply(bestPushDist, bestPushDir));
            }
        }
    }
}

void Building::ApplyDamage(int index, int damage, const Vector3& attackDir, float blowSpeed) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    if (inst.isDestroyed || inst.isBlownAway) return;

    inst.hp -= damage;
    if (inst.hp <= 0) {
        ScatterBuildingFloors(index, attackDir, blowSpeed);
    }
}

void Building::DestroyAt(int index, const Vector3& attackDir, float blowSpeed) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    if (inst.isDestroyed || inst.isBlownAway) return;

    ScatterBuildingFloors(index, attackDir, blowSpeed);
}

void Building::ScatterBuildingFloors(int index, const Vector3& attackDir, float blowSpeed) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    
    BuildingInstance baseInst = instances_[index];
    
    float fh = baseInst.scale.x * params_.floorHeightRatio;
    float totalHeight = baseInst.floorCount * fh;
    Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(baseInst.rotate);

    int initialCount = baseInst.floorCount;
    
    // 元のインスタンスは1階層分（一番下）として扱う
    instances_[index].floorCount = 1;
    instances_[index].hp = 0;
    instances_[index].isBlownAway = true;
    instances_[index].disappearTimer = 0.0f;

    for (int floor = 0; floor < initialCount; ++floor) {
        float localY = -totalHeight / 2.0f + fh * floor + fh / 2.0f;
        Vector3 localOffset = { 0.0f, localY, 0.0f };
        Vector3 rotatedOffset = {
            rotMat.m[0][0] * localOffset.x + rotMat.m[0][1] * localOffset.y + rotMat.m[0][2] * localOffset.z,
            rotMat.m[1][0] * localOffset.x + rotMat.m[1][1] * localOffset.y + rotMat.m[1][2] * localOffset.z,
            rotMat.m[2][0] * localOffset.x + rotMat.m[2][1] * localOffset.y + rotMat.m[2][2] * localOffset.z
        };
        Vector3 worldPos = Math::Add(baseInst.position, rotatedOffset);

        BuildingInstance* targetInst = nullptr;
        if (floor == 0) {
            targetInst = &instances_[index];
        } else {
            // 新しい階層を生成
            instances_.push_back(baseInst);
            targetInst = &instances_.back();

            targetInst->floorCount = 1;
            targetInst->hp = 0;
            targetInst->isBlownAway = true;
            targetInst->disappearTimer = 0.0f;
        }

        targetInst->position = worldPos;

        // 散弾銃のような広がりと上向きの力（各階層ごとにランダム）
        Vector3 dir = Math::Normalize(attackDir);
        float spread = params_.scatterSpread; // 破片が散らばるように強めの広がり
        dir.x += ((std::rand() % 100) / 100.0f - 0.5f) * spread;
        dir.z += ((std::rand() % 100) / 100.0f - 0.5f) * spread;
        dir.y += params_.scatterUpForceBase + ((std::rand() % 100) / 100.0f) * params_.scatterUpForceRand; // 上向きは控えめに
        dir = Math::Normalize(dir);

        // スピードも階層ごとに変える
        float randomSpeed = blowSpeed * (params_.scatterSpeedBase + ((std::rand() % 100) / 100.0f) * params_.scatterSpeedRand);
        targetInst->blowVelocity = Math::Multiply(randomSpeed, dir);

        // 激しい回転
        float angVel = params_.scatterAngularVelocity;
        targetInst->angularVelocity = { 
            ((std::rand() % 100) / 100.0f - 0.5f) * angVel, 
            ((std::rand() % 100) / 100.0f - 0.5f) * angVel, 
            ((std::rand() % 100) / 100.0f - 0.5f) * angVel 
        };
    }
}

void Building::ScatterAt(int index, const Vector3& velocity, const OBB& collisionArea) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    
    // 完全に破壊済みの場合は処理しない
    if (inst.isDestroyed) return;
    
    // ビル全体の高さと各階の高さを計算
    float floorHeight = inst.scale.x * params_.floorHeightRatio;
    float totalHeight = inst.floorCount * floorHeight;

    // 攻撃が当たったY座標から、どの階層が殴られたかを特定する
    float hitY = collisionArea.center.y;
    float bottomY = inst.position.y - totalHeight / 2.0f; // ビルの底面Y
    float localHitY = hitY - bottomY;
    
    int hitFloor = static_cast<int>(localHitY / floorHeight);
    if (hitFloor < 0) hitFloor = 0;
    if (hitFloor >= inst.floorCount) hitFloor = inst.floorCount - 1;

    // 殴られた階層の中心Y座標を計算
    float floorCenterY = bottomY + hitFloor * floorHeight + floorHeight / 2.0f;
    Vector3 floorPos = inst.position;
    floorPos.y = floorCenterY;

    // スケールはビル全体ではなく「1階層分のスケール」を使用し、ボクセルの密度を本来のモデル密度に戻す
    // 【重要】building.objはXZが[-1.0, 1.0]の幅2.0のモデルだが、
    // block.objはXZが[-0.5, 0.5]の幅1.0のモデルであるため、ボクセル側はXZスケールを2倍にする必要がある
    Vector3 floorScale = { inst.scale.x * 2.0f, floorHeight, inst.scale.z * 2.0f };

    engine_->GetVoxelParticleManager()->PlayCollisionScatter("building/block.obj", floorPos, velocity, inst.rotate, floorScale, collisionArea, VoxelParticleSystem::ParticleType::FineScatter);
}

bool Building::IsBuildingBlownAway(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    return instances_[index].isBlownAway && !instances_[index].isDestroyed;
}

bool Building::IsBuildingSpawning(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    return instances_[index].isSpawning;
}

bool Building::IsBuildingDestroyed(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    return instances_[index].isDestroyed;
}

Vector3 Building::GetBlowVelocity(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return {};
    return instances_[index].blowVelocity;
}

void Building::SetBlowVelocity(int index, const Vector3& v) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    instances_[index].blowVelocity = v;
}

Vector3 Building::GetBuildingPosition(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return {};
    return instances_[index].position;
}

void Building::MarkDestroyed(int index) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    if (inst.isDestroyed) return; // 既に破壊済み

    // 即爆散パーティクルを出す（まだ爆散していない場合）
    if (!inst.hasExploded) {
        Vector3 upDir = {0.0f, 1.0f, 0.0f}; // 上方向に爆散
        /// エネミーによる破壊は、重く黒焦げになる DebrisLargeGravity を指定
        engine_->GetVoxelParticleManager()->PlayExplosion("building/block.obj", inst.position, upDir, inst.rotate, inst.scale, VoxelParticleSystem::ParticleType::DebrisLargeGravity);
        inst.hasExploded = true;
    }

    inst.isDestroyed = true;
}

int Building::GetAliveBuildingCount() const {
    int alive = 0;
    for (const auto& inst : instances_) {
        if (!inst.isDestroyed && !inst.isBlownAway && inst.hp > 0) {
            alive++;
        }
    }
    return alive;
}

void Building::SpawnRandomBuilding(const Vector3& avoidPlayerPos, const Vector3& avoidBossPos) {
    OutputDebugStringA("Building: Spawning a new building dynamically...\n");

    // パラメータのバリデーション
    int minF = (std::max)(1, params_.minFloors);
    int maxF = (std::max)(minF, params_.maxFloors);
    float floorHRatio = (std::max)(0.1f, params_.floorHeightRatio);

    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<float> distPos(-params_.fieldRange, params_.fieldRange);
    std::uniform_int_distribution<int> distFloors(minF, maxF);
    std::uniform_real_distribution<float> distScaleXZ(params_.minScaleXZ, params_.maxScaleXZ);
    std::uniform_real_distribution<float> distRot(-3.14159265f, 3.14159265f);

    float scaleXZ = 0.0f;
    int floorCount = 0;
    float scaleY = 0.0f;
    Vector3 pos = { 0.0f, 0.0f, 0.0f };
    bool isValidPos = false;
    int maxAttempts = 100;

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        scaleXZ = distScaleXZ(engine);
        floorCount = distFloors(engine);
        float floorH = scaleXZ * floorHRatio;
        scaleY = floorCount * floorH;
        pos = { distPos(engine), scaleY / 2.0f, distPos(engine) };

        isValidPos = true;
        // building.objはXZ: -1.0~1.0 なので実際の半径 = scaleXZ * √2
        float radiusA = scaleXZ * 1.4142f;

        // 既存の建物との当たり判定
        for (const auto& inst : instances_) {
            if (inst.isDestroyed) continue;

            float radiusB = inst.scale.x * 1.4142f;
            float dx = pos.x - inst.position.x;
            float dz = pos.z - inst.position.z;
            float distanceSq = dx * dx + dz * dz;
            float requiredDistance = radiusA + radiusB + params_.minDistance;
            if (distanceSq < requiredDistance * requiredDistance) {
                isValidPos = false;
                break;
            }
        }

        if (!isValidPos) continue;

        // プレイヤーを避ける
        {
            float dx = pos.x - avoidPlayerPos.x;
            float dz = pos.z - avoidPlayerPos.z;
            float distanceSq = dx * dx + dz * dz;
            float requiredDistance = radiusA + params_.avoidPlayerRadius;
            if (distanceSq < requiredDistance * requiredDistance) {
                isValidPos = false;
            }
        }

        if (!isValidPos) continue;

        // ボスを避ける
        {
            float dx = pos.x - avoidBossPos.x;
            float dz = pos.z - avoidBossPos.z;
            float distanceSq = dx * dx + dz * dz;
            float requiredDistance = radiusA + params_.avoidBossRadius;
            if (distanceSq < requiredDistance * requiredDistance) {
                isValidPos = false;
            }
        }

        if (isValidPos) break;
    }

    if (!isValidPos) {
        OutputDebugStringA("Building: SpawnRandomBuilding failed to find a valid position.\n");
        return;
    }

    BuildingInstance inst;
    inst.scale = { scaleXZ, scaleY, scaleXZ };
    inst.floorCount = floorCount;
    inst.rotate = { 0.0f, distRot(engine), 0.0f };
    inst.hp = params_.buildingHp;
    inst.isBlownAway = false;
    inst.isDestroyed = false;
    inst.disappearTimer = 0.0f;
    inst.blowVelocity = {};
    inst.angularVelocity = {};

    // 出現演出初期化
    inst.isSpawning = true;
    inst.spawnTimer = 0.0f;
    
    // 出現にかかる時間 ＝ 高さ ／ 秒速
    if (params_.spawnSpeed > 0.0f) {
        inst.spawnDuration = scaleY / params_.spawnSpeed;
    } else {
        inst.spawnDuration = 2.0f; // ゼロ除算防止フォールバック
    }
    inst.targetPosition = pos;
    inst.initialY = pos.y - scaleY; // 完全に地中に埋まる高さ
    inst.position = { pos.x, inst.initialY, pos.z };



    instances_.push_back(std::move(inst));
    OutputDebugStringA("Building: Spawned a new building dynamically with spawning animation.\n");
}

