#include "SpriteRendererComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include <filesystem>
#include <algorithm>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "../Core/EditorDragDrop.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/IrufemiEngine.h"

void SpriteRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<SpriteRendererComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("SpriteRenderer", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        TextureManager* tm = Sprite::GetTextureManager();
        if (tm) {
            auto names = tm->GetTextureNamesForDebug();
            int currentIndex = 0;
            for (int i = 0; i < (int)names.size(); ++i) {
                if (names[i] == comp->texturePath_) {
                    currentIndex = i;
                    break;
                }
            }
            const char* currentPreview = names.empty() ? "" : names[currentIndex].c_str();
            if (ImGui::BeginCombo("Texture", currentPreview)) {
                for (int i = 0; i < names.size(); ++i) {
                    bool isSelected = (currentIndex == i);
                    if (ImGui::Selectable(names[i].c_str(), isSelected)) {
                        std::string oldTex = comp->texturePath_;
                        std::string newTex = names[i];
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTex, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        } else {
            char buffer[256];
            strncpy_s(buffer, sizeof(buffer), comp->texturePath_.c_str(), _TRUNCATE);
            static std::string startTex;
            if (ImGui::InputText("TexturePath", buffer, sizeof(buffer))) {
                comp->SetTexture(buffer);
            }
            if (ImGui::IsItemActivated()) startTex = comp->texturePath_;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                std::string endTex = buffer;
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                    startTex, endTex, [comp](const std::string& v){ comp->SetTexture(v); }));
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
                        newTexName = newTexName.substr(18); // length of "resources/texture/"
                    } else if (lowerPath.find("resources/") == 0) {
                        newTexName = newTexName.substr(10);
                    }
                    std::string oldTex = comp->texturePath_;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTexName, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        bool isTopMost = comp->isTopMost_;
        if (ImGui::Checkbox("TopMost (Draw over 3D)", &isTopMost)) {
            ComponentUIHelpers::PushInstantUndo(actionManager, comp->isTopMost_, isTopMost, std::function<void(const bool&)>([comp](const bool& v){ comp->isTopMost_ = v; if (comp->GetSprite()) comp->GetSprite()->SetTopMost(v); }));
        }
        bool isFlipX = comp->isFlipX_;
        if (ImGui::Checkbox("Flip X", &isFlipX)) {
            ComponentUIHelpers::PushInstantUndo(actionManager, comp->isFlipX_, isFlipX, std::function<void(const bool&)>([comp](const bool& v){ comp->isFlipX_ = v; if (comp->GetSprite()) comp->GetSprite()->SetFlip(comp->isFlipX_, comp->isFlipY_); }));
        }
        ImGui::SameLine();
        bool isFlipY = comp->isFlipY_;
        if (ImGui::Checkbox("Flip Y", &isFlipY)) {
            ComponentUIHelpers::PushInstantUndo(actionManager, comp->isFlipY_, isFlipY, std::function<void(const bool&)>([comp](const bool& v){ comp->isFlipY_ = v; if (comp->GetSprite()) comp->GetSprite()->SetFlip(comp->isFlipX_, comp->isFlipY_); }));
        }

        if (ImGui::SliderFloat2("Anchor", comp->anchor_, 0.0f, 1.0f)) {
            if (comp->GetSprite()) comp->GetSprite()->SetAnchor(comp->anchor_[0], comp->anchor_[1]);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, reinterpret_cast<Vector2*>(comp->anchor_), std::function<void(const Vector2&)>([comp](const Vector2& v){ 
            comp->anchor_[0] = v.x; comp->anchor_[1] = v.y; 
            if (comp->GetSprite()) comp->GetSprite()->SetAnchor(v.x, v.y); 
        }));
        
        ImGui::DragFloat2("Base Size", comp->size_, 1.0f, 1.0f, 8192.0f);
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, reinterpret_cast<Vector2*>(comp->size_), std::function<void(const Vector2&)>([comp](const Vector2& v){
            comp->size_[0] = v.x; comp->size_[1] = v.y;
        }));
        
        if (ImGui::ColorEdit4("Color", &comp->color_.x)) {
            if (comp->GetSprite()) comp->GetSprite()->SetColor(comp->color_);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->color_, std::function<void(const Vector4&)>([comp](const Vector4& v){
            comp->color_ = v;
            if (comp->GetSprite()) comp->GetSprite()->SetColor(v);
        }));
        ImGui::TreePop();
    }
}
#endif // EditorMode
