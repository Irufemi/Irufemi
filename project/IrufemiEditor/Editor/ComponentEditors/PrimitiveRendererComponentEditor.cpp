#include "PrimitiveRendererComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"

void PrimitiveRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<PrimitiveRendererComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("PrimitiveRenderer", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        const char* typeNames[] = {
            "Triangle", "Plane", "Cube", "Cylinder", "Sphere", 
            "Tetra", "Circle", "Ring", "Skybox", "Cone", 
            "Torus", "IcoSphere", "Grid"
        };
        
        int typeIndex = comp->currentTypeIndex_;
        int oldTypeIndex = typeIndex;
        if (ImGui::Combo("Shape Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
            ComponentUIHelpers::PushInstantUndo(actionManager, oldTypeIndex, typeIndex, std::function<void(const int&)>([comp](const int& v) { comp->SetShape(static_cast<PrimitiveType>(v)); comp->RebuildMesh(); }));
        }

        PrimitiveType type = static_cast<PrimitiveType>(comp->currentTypeIndex_);
        switch (type) {
            case PrimitiveType::Sphere:
            case PrimitiveType::IcoSphere:
            case PrimitiveType::Circle:
                if (ImGui::DragFloat("Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->radius_, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                if (ImGui::SliderInt("Subdivisions", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                break;
            case PrimitiveType::Cylinder:
                if (ImGui::DragFloat("Top Radius", &comp->topRadius_, 0.1f, 0.0f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->topRadius_, std::function<void(const float&)>([comp](const float& v){ comp->topRadius_ = v; comp->RebuildMesh(); }));
                if (ImGui::DragFloat("Bottom Radius", &comp->bottomRadius_, 0.1f, 0.0f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->bottomRadius_, std::function<void(const float&)>([comp](const float& v){ comp->bottomRadius_ = v; comp->RebuildMesh(); }));
                if (ImGui::DragFloat("Height", &comp->height_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->height_, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                if (ImGui::SliderInt("Segments", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                
                {
                    bool hasTop = comp->hasTop_;
                    if (ImGui::Checkbox("Has Top", &hasTop)) {
                        ComponentUIHelpers::PushInstantUndo(actionManager, comp->hasTop_, hasTop, std::function<void(const bool&)>([comp](const bool& v){ comp->hasTop_ = v; comp->RebuildMesh(); }));
                    }
                    bool hasBottom = comp->hasBottom_;
                    if (ImGui::Checkbox("Has Bottom", &hasBottom)) {
                        ComponentUIHelpers::PushInstantUndo(actionManager, comp->hasBottom_, hasBottom, std::function<void(const bool&)>([comp](const bool& v){ comp->hasBottom_ = v; comp->RebuildMesh(); }));
                    }
                }
                break;
            case PrimitiveType::Cone:
                if (ImGui::DragFloat("Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->radius_, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                if (ImGui::DragFloat("Height", &comp->height_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->height_, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                if (ImGui::SliderInt("Segments", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                break;
            case PrimitiveType::Torus:
                if (ImGui::DragFloat("Major Radius", &comp->torusMajorRadius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMajorRadius_, std::function<void(const float&)>([comp](const float& v){ comp->torusMajorRadius_ = v; comp->RebuildMesh(); }));
                if (ImGui::DragFloat("Minor Radius", &comp->torusMinorRadius_, 0.05f, 0.01f, 100.0f)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMinorRadius_, std::function<void(const float&)>([comp](const float& v){ comp->torusMinorRadius_ = v; comp->RebuildMesh(); }));
                if (ImGui::SliderInt("Major Segments", &comp->torusMajorSegments_, 3, 64)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMajorSegments_, std::function<void(const int&)>([comp](const int& v){ comp->torusMajorSegments_ = v; comp->RebuildMesh(); }));
                if (ImGui::SliderInt("Minor Segments", &comp->torusMinorSegments_, 3, 64)) comp->RebuildMesh();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMinorSegments_, std::function<void(const int&)>([comp](const int& v){ comp->torusMinorSegments_ = v; comp->RebuildMesh(); }));
                break;
        }
        ImGui::TreePop();
    }
}
#endif // EditorMode
