#include "Primitive2DRendererComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include <filesystem>
#include <algorithm>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Renderer/Primitive2DRendererComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "../Core/EditorDragDrop.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h" // TextureManager取得用（共通）

void Primitive2DRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<Primitive2DRendererComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("Primitive2DRenderer", ImGuiTreeNodeFlags_DefaultOpen);

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
            "Rect", "Triangle", "Circle", "Ring", "Line"
        };
        
        int typeIndex = static_cast<int>(comp->GetPrimitive()->GetShape());
        int oldTypeIndex = typeIndex;
        if (ImGui::Combo("Shape Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
            ComponentUIHelpers::PushInstantUndo(actionManager, oldTypeIndex, typeIndex, std::function<void(const int&)>([comp](const int& v) { comp->SetShape(static_cast<Primitive2DType>(v)); }));
        }

        Primitive2DType type = static_cast<Primitive2DType>(typeIndex);
        
        // Size
        Vector2 size = comp->GetPrimitive()->GetSize();
        if (ImGui::DragFloat2("Size", &size.x, 1.0f, 0.0f, 10000.0f)) {
            comp->SetSize(size);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &size, std::function<void(const Vector2&)>([comp](const Vector2& v){ comp->SetSize(v); }));

        // Pivot
        Vector2 pivot = comp->GetPrimitive()->GetPivot();
        if (ImGui::DragFloat2("Pivot", &pivot.x, 0.01f, 0.0f, 1.0f)) {
            comp->SetPivot(pivot);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &pivot, std::function<void(const Vector2&)>([comp](const Vector2& v){ comp->SetPivot(v); }));

        switch (type) {
            case Primitive2DType::Circle:
            case Primitive2DType::Ring:
                {
                    int sub = comp->GetPrimitive()->GetSubdivision();
                    if (ImGui::SliderInt("Subdivision", &sub, 3, 128)) {
                        comp->SetSubdivision(sub);
                    }
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &sub, std::function<void(const int&)>([comp](const int& v){ comp->SetSubdivision(v); }));
                }
                break;
            default:
                break;
        }

        switch (type) {
            case Primitive2DType::Ring:
            case Primitive2DType::Line:
                {
                    float thick = comp->GetPrimitive()->GetThickness();
                    if (ImGui::DragFloat("Thickness", &thick, 0.1f, 0.1f, 100.0f)) {
                        comp->SetThickness(thick);
                    }
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &thick, std::function<void(const float&)>([comp](const float& v){ comp->SetThickness(v); }));
                }
                break;
            default:
                break;
        }

        ImGui::Separator();
        ImGui::Text("Appearance");

        // TopMost
        bool topMost = comp->GetPrimitive()->IsTopMost();
        if (ImGui::Checkbox("TopMost", &topMost)) {
            ComponentUIHelpers::PushInstantUndo(actionManager, comp->GetPrimitive()->IsTopMost(), topMost, std::function<void(const bool&)>([comp](const bool& v){ comp->SetTopMost(v); }));
        }
        
        // Color
        Vector4 color = comp->GetPrimitive()->GetColor();
        if (ImGui::ColorEdit4("Color", &color.x)) {
            comp->SetColor(color);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &color, std::function<void(const Vector4&)>([comp](const Vector4& v){ comp->SetColor(v); }));

        // Texture
        TextureManager* tm = Primitive3DObject::GetTextureManager(); // TextureManagerを共通で取得
        if (tm) {
            auto names = tm->GetTextureNamesForDebug();
            int currentIndex = 0;
            // Handle valid ResourceHandle to get the name, but our component stores texturePath directly
            std::string currentTex = comp->GetPrimitive()->GetTextureHandle().IsValid() ? names[comp->GetPrimitive()->GetTextureHandle().index] : "";
            for (int i = 0; i < (int)names.size(); ++i) {
                if (names[i] == currentTex) {
                    currentIndex = i;
                    break;
                }
            }
            const char* currentPreview = names.empty() ? "" : names[currentIndex].c_str();
            if (ImGui::BeginCombo("Texture", currentPreview)) {
                for (int i = 0; i < names.size(); ++i) {
                    bool isSelected = (currentIndex == i);
                    if (ImGui::Selectable(names[i].c_str(), isSelected)) {
                        std::string oldTex = currentTex;
                        std::string newTex = names[i];
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTex, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadAssetPath)) {
                std::string droppedPathStr = static_cast<const char*>(payload->Data);
                std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                std::string ext = droppedPath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") {
                    std::string newTexName = droppedPathStr;
                    std::replace(newTexName.begin(), newTexName.end(), '\\', '/');
                    std::string lowerPath = newTexName;
                    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                    if (lowerPath.find("resources/texture/") == 0) {
                        newTexName = newTexName.substr(18);
                    } else if (lowerPath.find("resources/") == 0) {
                        newTexName = newTexName.substr(10);
                    }
                    
                    std::string oldTex = comp->GetPrimitive()->GetTextureHandle().IsValid() ? tm->GetTextureNamesForDebug()[comp->GetPrimitive()->GetTextureHandle().index] : "";
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTexName, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
