#include "building.h"
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <format>
#include <Windows.h>
#include <cmath>

#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Renderer/VoxelParticle/VoxelParticleSystem.h"
#include "Engine/IrufemiEngine.h"

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

using json = nlohmann::json;

Building::Building() {}

Building::~Building() {}

void Building::Initialize(Camera* camera, IrufemiEngine* engine) {
    camera_ = camera;
    engine_ = engine;

    LoadJson();
    Generate();

#ifdef USE_IMGUI
    if (engine_) {
        debugLines_ = std::make_unique<Line3DRegion>();
        debugLines_->Initialize(camera_);
    }
#endif
}

void Building::Update() {
    for (auto& inst : instances_) {
        // 完全消滅済みかつボクセルも終了しているならスキップ
        if (inst.isDestroyed) {
            // ボクセルパーティクルがまだアクティブなら更新を続ける
            if (inst.voxelSystem && inst.voxelSystem->IsActive()) {
                inst.voxelSystem->Update(1.0f / 60.0f);
            }
            continue;
        }

        if (inst.isBlownAway) {
            // 吹き飛び中の移動
            inst.position = Math::Add(inst.position, inst.blowVelocity);
            inst.rotate.x += inst.angularVelocity.x;
            inst.rotate.y += inst.angularVelocity.y;
            inst.rotate.z += inst.angularVelocity.z;

            // 壁反射
            const float bound = BuildingInstance::kFieldBound;
            const float r = inst.scale.x * 0.5f;
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

            // ボクセル爆散トリガー（消滅タイマーが閾値を超えた瞬間）
            if (!inst.hasExploded && prevTimer < BuildingInstance::kDisappearTime &&
                inst.disappearTimer >= BuildingInstance::kDisappearTime) {
                if (inst.voxelSystem) {
                    inst.voxelSystem->Explode(inst.position, inst.blowVelocity,
                                             inst.rotate, inst.scale);
                    inst.hasExploded = true;
                }
            }

            // 完全消滅判定（モデル消滅 + ボクセルパーティクル終了）
            if (inst.disappearTimer >= BuildingInstance::kDisappearTime) {
                bool voxelActive = inst.voxelSystem && inst.voxelSystem->IsActive();
                if (!voxelActive) {
                    inst.isDestroyed = true;
                    continue;
                }
            }
        }

        // ObjClassの座標を同期
        if (inst.obj) {
            inst.obj->SetPosition(inst.position);
            inst.obj->SetRotate(inst.rotate);
            inst.obj->SetScale(inst.scale);
            inst.obj->Update();
        }

        // VoxelParticleSystemの更新
        if (inst.voxelSystem) {
            inst.voxelSystem->Update(1.0f / 60.0f);
        }
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
}

void Building::Draw(IrufemiEngine* engine) {
    for (auto& inst : instances_) {
        // 完全消滅済みでもボクセルパーティクルがあれば描画
        if (inst.isDestroyed) {
            if (inst.voxelSystem && inst.voxelSystem->IsActive()) {
                inst.voxelSystem->Draw();
            }
            continue;
        }

        // モデル描画（消滅タイマー前のみ）
        bool modelGone = inst.isBlownAway && inst.disappearTimer >= BuildingInstance::kDisappearTime;
        if (inst.obj && !modelGone) {
            engine->SetBlend(BlendMode::kBlendModeNormal);
            engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine->SetCull(PSOManager::CullMode::Back);
            inst.obj->Draw();
        }

        // VoxelParticleSystemの描画
        if (inst.voxelSystem) {
            engine->SetBlend(BlendMode::kBlendModeNormal);
            engine->SetDepthWrite(PSOManager::DepthWrite::Enable);
            engine->SetCull(PSOManager::CullMode::Back);
            inst.voxelSystem->Draw();
        }
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
    if (ImGui::Begin("Building Settings")) {
        bool changed = false;

        changed |= ImGui::SliderInt("Count", &params_.count, 1, 100);
        changed |= ImGui::DragFloat("Field Range", &params_.fieldRange, 1.0f, 10.0f, 200.0f);
        changed |= ImGui::DragFloat("Min Height", &params_.minHeight, 0.1f, 0.1f, 100.0f);
        changed |= ImGui::DragFloat("Max Height", &params_.maxHeight, 0.1f, 0.1f, 100.0f);
        changed |= ImGui::DragFloat("Min Scale XZ", &params_.minScaleXZ, 0.1f, 0.1f, 50.0f);
        changed |= ImGui::DragFloat("Max Scale XZ", &params_.maxScaleXZ, 0.1f, 0.1f, 50.0f);
        changed |= ImGui::DragFloat("Min Distance", &params_.minDistance, 0.1f, 0.0f, 100.0f);
        changed |= ImGui::DragInt("Building HP", &params_.buildingHp, 1, 1, 10000);

        // 最小値と最大値の整合性を保つ
        if (params_.minHeight > params_.maxHeight) {
            params_.maxHeight = params_.minHeight;
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
        int alive = 0;
        for (auto& inst : instances_) {
            if (!inst.isDestroyed && !inst.isBlownAway) alive++;
        }
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
        if (b.contains("min_height")) params_.minHeight = b["min_height"];
        if (b.contains("max_height")) params_.maxHeight = b["max_height"];
        if (b.contains("min_scale_xz")) params_.minScaleXZ = b["min_scale_xz"];
        if (b.contains("max_scale_xz")) params_.maxScaleXZ = b["max_scale_xz"];
        if (b.contains("field_range")) params_.fieldRange = b["field_range"];
        if (b.contains("min_distance")) params_.minDistance = b["min_distance"];
        if (b.contains("building_hp")) params_.buildingHp = b["building_hp"];
        OutputDebugStringA("Building: Parameters loaded from JSON.\n");
    }
}

void Building::SaveJson() {
    json j;
    j["building"] = {
        {"count", params_.count},
        {"min_height", params_.minHeight},
        {"max_height", params_.maxHeight},
        {"min_scale_xz", params_.minScaleXZ},
        {"max_scale_xz", params_.maxScaleXZ},
        {"field_range", params_.fieldRange},
        {"min_distance", params_.minDistance},
        {"building_hp", params_.buildingHp}
    };

    std::ofstream file(kJsonFilePath);
    if (file.is_open()) {
        file << j.dump(4);
        OutputDebugStringA("Building: Parameters saved to JSON.\n");
    }
}

void Building::Generate() {
    OutputDebugStringA("Building: Regenerating buildings...\n");
    instances_.clear();

    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<float> distPos(-params_.fieldRange, params_.fieldRange);
    std::uniform_real_distribution<float> distHeight(params_.minHeight, params_.maxHeight);
    std::uniform_real_distribution<float> distScaleXZ(params_.minScaleXZ, params_.maxScaleXZ);
    std::uniform_real_distribution<float> distRot(-3.14159265f, 3.14159265f);

    // 配置候補の位置・スケールを先に決めてから ObjClass を生成する
    struct PlacementData {
        Vector3 pos;
        Vector3 scale;
        float rotY;
    };
    std::vector<PlacementData> placements;

    for (int i = 0; i < params_.count; ++i) {
        float scaleXZ = 0.0f;
        float scaleY = 0.0f;
        Vector3 pos = { 0.0f, 0.0f, 0.0f };
        bool isValidPos = false;
        int maxAttempts = 100;

        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            scaleXZ = distScaleXZ(engine);
            scaleY = distHeight(engine);
            pos = { distPos(engine), scaleY / 2.0f, distPos(engine) };

            isValidPos = true;
            float radiusA = scaleXZ * 0.7071f;

            for (const auto& p : placements) {
                float radiusB = p.scale.x * 0.7071f;
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

        placements.push_back({ pos, { scaleXZ, scaleY, scaleXZ }, distRot(engine) });
    }

    // 実際にインスタンスを生成
    for (auto& pd : placements) {
        BuildingInstance inst;
        inst.obj = std::make_unique<ObjClass>();
        inst.obj->Initialize(camera_, "building/block.obj");
        inst.position = pd.pos;
        inst.scale = pd.scale;
        inst.rotate = { 0.0f, pd.rotY, 0.0f };
        inst.hp = params_.buildingHp;
        inst.isBlownAway = false;
        inst.isDestroyed = false;
        inst.disappearTimer = 0.0f;
        inst.blowVelocity = {};
        inst.angularVelocity = {};

        inst.obj->SetPosition(inst.position);
        inst.obj->SetScale(inst.scale);
        inst.obj->SetRotate(inst.rotate);

        instances_.push_back(std::move(inst));
    }

    // VoxelParticleSystemの初期化（GPUリソース作成はメインスレッドで行う必要がある）
    for (auto& inst : instances_) {
        inst.voxelSystem = std::make_unique<VoxelParticleSystem>();
        inst.voxelSystem->Initialize("building/block.obj", {32, 32, 32}, camera_);
        inst.voxelSystem->SetParticleType(VoxelParticleSystem::ParticleType::Building);
        inst.voxelSystem->SetGravity(40.0f); // 落下感を強くするため重力を上げる
    }

    OutputDebugStringA(std::format("Building: {} buildings generated.\n", (int)instances_.size()).c_str());
}

// --- 当たり判定用 ---

bool Building::IsBuildingAlive(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    const auto& inst = instances_[index];
    return !inst.isDestroyed && !inst.isBlownAway && inst.hp > 0;
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
    obb.size = { inst.scale.x * 0.5f, inst.scale.y * 0.5f, inst.scale.z * 0.5f };

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
        inst.hp = 0;
        inst.isBlownAway = true;
        inst.disappearTimer = 0.0f;

        Vector3 dir = Math::Normalize(attackDir);
        inst.blowVelocity = Math::Multiply(blowSpeed, dir);
        inst.blowVelocity.y = 0.0f;

        // 回転を加える
        inst.angularVelocity = { 0.05f, 0.1f, 0.03f };
    }
}

void Building::DestroyAt(int index, const Vector3& attackDir, float blowSpeed) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    if (inst.isDestroyed || inst.isBlownAway) return;

    inst.hp = 0;
    inst.isBlownAway = true;
    inst.disappearTimer = 0.0f;

    Vector3 dir = Math::Normalize(attackDir);
    inst.blowVelocity = Math::Multiply(blowSpeed, dir);
    inst.blowVelocity.y = 0.0f;

    inst.angularVelocity = { 0.08f, 0.15f, 0.05f };
}

void Building::ScatterAt(int index, const Vector3& velocity, const OBB& collisionArea) {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return;
    auto& inst = instances_[index];
    
    // 完全に破壊済み、またはVoxelシステムがない場合は処理しない
    if (inst.isDestroyed || !inst.voxelSystem) return;
    
    inst.voxelSystem->CollisionScatter(inst.position, velocity, inst.rotate, inst.scale, collisionArea);
}

bool Building::IsBuildingBlownAway(int index) const {
    if (index < 0 || index >= static_cast<int>(instances_.size())) return false;
    return instances_[index].isBlownAway && !instances_[index].isDestroyed;
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
    if (!inst.hasExploded && inst.voxelSystem) {
        Vector3 upDir = {0.0f, 1.0f, 0.0f}; // 上方向に爆散
        inst.voxelSystem->Explode(inst.position, upDir, inst.rotate, inst.scale);
        inst.hasExploded = true;
    }

    inst.isDestroyed = true;
}

