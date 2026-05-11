#include "PrimitiveRendererComponent.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "Renderer/Object3D/Primitive/PrimitiveObjects3DClass.h"
#include "Engine/Manager/PrimitiveManager.h"
#include "Engine/Core/Type/PrimitiveType.h"

PrimitiveRendererComponent::PrimitiveRendererComponent() {}
PrimitiveRendererComponent::~PrimitiveRendererComponent() {}

void PrimitiveRendererComponent::Initialize() {
    primitive_ = std::make_unique<PrimitiveObjects3DClass>();
    // 初期値として Cube を生成
    primitive_->Initialize(PrimitiveType::Cube);

    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void PrimitiveRendererComponent::Update() {
    if (transform_ && primitive_) {
        primitive_->SetPosition(transform_->position_);
        primitive_->SetRotate(transform_->rotation_);
        primitive_->SetScale(transform_->scale_);
    }

    if (primitive_) {
        primitive_->Update();
    }
}

void PrimitiveRendererComponent::Draw() {
    if (primitive_) {
        primitive_->Draw();
    }
}

void PrimitiveRendererComponent::SetShape(PrimitiveType type) {
    if (primitive_) {
        primitive_->SetShape(type);
        currentTypeIndex_ = static_cast<int>(type);
    }
}

void PrimitiveRendererComponent::SetColor(const Vector4& color) {
    if (primitive_) {
        primitive_->SetColor(color);
    }
}

void PrimitiveRendererComponent::SetTexture(const std::string& texturePath) {
    if (primitive_) {
        primitive_->SetTexture(texturePath);
    }
}

void PrimitiveRendererComponent::RebuildMesh() {
    if (!primitive_) return;
    
    PrimitiveType type = static_cast<PrimitiveType>(currentTypeIndex_);
    PrimitiveData data;

    switch (type) {
        case PrimitiveType::Sphere:
        case PrimitiveType::IcoSphere:
            data = PrimitiveManager::CreateSphere(radius_, subdivisions_);
            break;
        case PrimitiveType::Cylinder:
            data = PrimitiveManager::CreateCylinder(bottomRadius_, topRadius_, height_, subdivisions_, hasTop_, hasBottom_);
            break;
        case PrimitiveType::Cone:
            data = PrimitiveManager::CreateCone(radius_, height_, subdivisions_);
            break;
        case PrimitiveType::Torus:
            data = PrimitiveManager::CreateTorus(torusMajorRadius_, torusMinorRadius_, torusMajorSegments_, torusMinorSegments_);
            break;
        case PrimitiveType::Circle:
            data = PrimitiveManager::CreateCircle(radius_, subdivisions_);
            break;
        case PrimitiveType::Cube:
        case PrimitiveType::Plane:
        case PrimitiveType::Triangle:
        case PrimitiveType::Tetra:
        default:
            // これらの基本図形は標準リソースに戻す
            primitive_->SetShape(type);
            return;
    }
    
    primitive_->ReinitializeMesh(data);
}

#ifdef EditorMode
#include <imgui.h>
void PrimitiveRendererComponent::OnInspectorGUI() {
    if (ImGui::TreeNodeEx("PrimitiveRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        const char* typeNames[] = {
            "Triangle", "Plane", "Cube", "Cylinder", "Sphere", 
            "Tetra", "Circle", "Ring", "Skybox", "Cone", 
            "Torus", "IcoSphere", "Grid"
        };
        
        bool needRebuild = false;

        if (ImGui::Combo("Shape Type", &currentTypeIndex_, typeNames, IM_ARRAYSIZE(typeNames))) {
            primitive_->SetShape(static_cast<PrimitiveType>(currentTypeIndex_));
            // 形状が切り替わったらとりあえず標準設定に戻しつつリビルド判定も兼ねる
            needRebuild = true;
        }

        PrimitiveType type = static_cast<PrimitiveType>(currentTypeIndex_);

        // 形状ごとの詳細パラメータ UI
        switch (type) {
            case PrimitiveType::Sphere:
            case PrimitiveType::IcoSphere:
            case PrimitiveType::Circle:
                if (ImGui::DragFloat("Radius", &radius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                if (ImGui::SliderInt("Subdivisions", &subdivisions_, 3, 64)) needRebuild = true;
                break;
                
            case PrimitiveType::Cylinder:
                if (ImGui::DragFloat("Top Radius", &topRadius_, 0.1f, 0.0f, 100.0f)) needRebuild = true;
                if (ImGui::DragFloat("Bottom Radius", &bottomRadius_, 0.1f, 0.0f, 100.0f)) needRebuild = true;
                if (ImGui::DragFloat("Height", &height_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                if (ImGui::SliderInt("Segments", &subdivisions_, 3, 64)) needRebuild = true;
                if (ImGui::Checkbox("Has Top", &hasTop_)) needRebuild = true;
                if (ImGui::Checkbox("Has Bottom", &hasBottom_)) needRebuild = true;
                break;
                
            case PrimitiveType::Cone:
                if (ImGui::DragFloat("Radius", &radius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                if (ImGui::DragFloat("Height", &height_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                if (ImGui::SliderInt("Segments", &subdivisions_, 3, 64)) needRebuild = true;
                break;
                
            case PrimitiveType::Torus:
                if (ImGui::DragFloat("Major Radius", &torusMajorRadius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                if (ImGui::DragFloat("Minor Radius", &torusMinorRadius_, 0.05f, 0.01f, 100.0f)) needRebuild = true;
                if (ImGui::SliderInt("Major Segments", &torusMajorSegments_, 3, 64)) needRebuild = true;
                if (ImGui::SliderInt("Minor Segments", &torusMinorSegments_, 3, 64)) needRebuild = true;
                break;
        }

        if (needRebuild) {
            RebuildMesh();
        }
        
        ImGui::TreePop();
    }
}
#endif

nlohmann::json PrimitiveRendererComponent::Serialize() {
    nlohmann::json j;
    j["currentTypeIndex"] = currentTypeIndex_;
    j["radius"] = radius_;
    j["subdivisions"] = subdivisions_;
    j["height"] = height_;
    j["topRadius"] = topRadius_;
    j["bottomRadius"] = bottomRadius_;
    j["hasTop"] = hasTop_;
    j["hasBottom"] = hasBottom_;
    j["torusMajorRadius"] = torusMajorRadius_;
    j["torusMinorRadius"] = torusMinorRadius_;
    j["torusMajorSegments"] = torusMajorSegments_;
    j["torusMinorSegments"] = torusMinorSegments_;
    return j;
}

void PrimitiveRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("currentTypeIndex")) currentTypeIndex_ = j["currentTypeIndex"];
    if (j.contains("radius")) radius_ = j["radius"];
    if (j.contains("subdivisions")) subdivisions_ = j["subdivisions"];
    if (j.contains("height")) height_ = j["height"];
    if (j.contains("topRadius")) topRadius_ = j["topRadius"];
    if (j.contains("bottomRadius")) bottomRadius_ = j["bottomRadius"];
    if (j.contains("hasTop")) hasTop_ = j["hasTop"];
    if (j.contains("hasBottom")) hasBottom_ = j["hasBottom"];
    if (j.contains("torusMajorRadius")) torusMajorRadius_ = j["torusMajorRadius"];
    if (j.contains("torusMinorRadius")) torusMinorRadius_ = j["torusMinorRadius"];
    if (j.contains("torusMajorSegments")) torusMajorSegments_ = j["torusMajorSegments"];
    if (j.contains("torusMinorSegments")) torusMinorSegments_ = j["torusMinorSegments"];
    
    // 形状を再構築
    PrimitiveType types[] = { PrimitiveType::Sphere, PrimitiveType::Plane, PrimitiveType::Cube, PrimitiveType::Cylinder, PrimitiveType::Cone, PrimitiveType::Torus };
    if (currentTypeIndex_ >= 0 && currentTypeIndex_ < 6) {
        SetShape(types[currentTypeIndex_]);
    }
}
