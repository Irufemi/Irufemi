#include "building.h"
#include <fstream>
#include <random>
#include <nlohmann/json.hpp>
#include <format>
#include <Windows.h>

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
    for (auto& building : buildings_) {
        building->Update();
    }

#ifdef USE_IMGUI
    if (engine_ && engine_->GetInputManager()->IsKeyPressedDIK(0x3B /*DIK_F1*/)) {
        isDebugDraw_ = !isDebugDraw_;
    }

    if (debugLines_) {
        debugLines_->ClearInstances();
        if (isDebugDraw_) {
            Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // Green

            for (auto& b : buildings_) {
                Vector3 scale = b->GetScale();
                Vector3 pos = b->GetPosition();
                Vector3 rot = b->GetRotate();

                Matrix4x4 rotMat = Math::MakeRotateYMatrix(rot.y);

                Vector3 corners[8];
                // 底部4点
                corners[0] = { -0.5f, -0.5f, -0.5f };
                corners[1] = { 0.5f, -0.5f, -0.5f };
                corners[2] = { 0.5f, -0.5f, 0.5f };
                corners[3] = { -0.5f, -0.5f, 0.5f };
                // 上部4点
                corners[4] = { -0.5f, 0.5f, -0.5f };
                corners[5] = { 0.5f, 0.5f, -0.5f };
                corners[6] = { 0.5f, 0.5f, 0.5f };
                corners[7] = { -0.5f, 0.5f, 0.5f };

                for (int i = 0; i < 8; ++i) {
                    corners[i].x *= scale.x;
                    corners[i].y *= scale.y;
                    corners[i].z *= scale.z;
                    corners[i] = Math::Transform(corners[i], rotMat);
                    corners[i] = Math::Add(corners[i], pos);
                }

                // 底面の辺
                debugLines_->AddInstance(corners[0], corners[1], color);
                debugLines_->AddInstance(corners[1], corners[2], color);
                debugLines_->AddInstance(corners[2], corners[3], color);
                debugLines_->AddInstance(corners[3], corners[0], color);
                // 上面の辺
                debugLines_->AddInstance(corners[4], corners[5], color);
                debugLines_->AddInstance(corners[5], corners[6], color);
                debugLines_->AddInstance(corners[6], corners[7], color);
                debugLines_->AddInstance(corners[7], corners[4], color);
                // 垂直の辺
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

void Building::Draw() {
    for (auto& building : buildings_) {
        building->Draw();
    }

#ifdef USE_IMGUI
    if (debugLines_ && isDebugDraw_ && engine_) {
        engine_->ApplyLineInstancedPSO();
        debugLines_->Draw();
        engine_->ApplyPSO();
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
        {"field_range", params_.fieldRange}
    };

    std::ofstream file(kJsonFilePath);
    if (file.is_open()) {
        file << j.dump(4);
        OutputDebugStringA("Building: Parameters saved to JSON.\n");
    }
}

void Building::Generate() {
    OutputDebugStringA("Building: Regenerating buildings...\n");
    buildings_.clear();

    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<float> distPos(-params_.fieldRange, params_.fieldRange);
    std::uniform_real_distribution<float> distHeight(params_.minHeight, params_.maxHeight);
    std::uniform_real_distribution<float> distScaleXZ(params_.minScaleXZ, params_.maxScaleXZ);
    std::uniform_real_distribution<float> distRot(-3.14159265f, 3.14159265f);

    for (int i = 0; i < params_.count; ++i) {
        auto b = std::make_unique<ObjClass>();
        b->Initialize(camera_, "building/block.obj");

        float scaleXZ = distScaleXZ(engine);
        float scaleY = distHeight(engine);
        b->SetScale({ scaleXZ, scaleY, scaleXZ });

        b->SetRotate({ 0.0f, distRot(engine), 0.0f });

        b->SetPosition({ distPos(engine), scaleY / 2.0f, distPos(engine) });
        
        buildings_.push_back(std::move(b));
    }
    OutputDebugStringA(std::format("Building: {} buildings generated.\n", (int)buildings_.size()).c_str());
}
