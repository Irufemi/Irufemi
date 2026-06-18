#include "TextRendererComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "Engine/Graphics/Font/FontManager.h"
#include "Engine/Core/Utility/StringUtility.h"

void TextRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<TextRendererComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("TextRenderer", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        // Text (UTF-8)
        std::string utf8Text = ConvertString(comp->GetText());
        char textBuffer[256];
        strncpy_s(textBuffer, sizeof(textBuffer), utf8Text.c_str(), _TRUNCATE);
        static std::string startText;
        if (ImGui::InputText("Text", textBuffer, sizeof(textBuffer))) {
            comp->SetText(ConvertString(std::string(textBuffer)));
        }
        if (ImGui::IsItemActivated()) startText = utf8Text;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            std::string endText = textBuffer;
            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                startText, endText, [comp](const std::string& v){ comp->SetText(ConvertString(v)); }));
        }

        // Font ID
        std::string fontId = comp->GetFontId();
        FontManager* fm = Text::GetFontManager();
        if (fm) {
            auto fontIds = fm->GetLoadedFontIds();
            int currentIndex = 0;
            for (int i = 0; i < (int)fontIds.size(); ++i) {
                if (fontIds[i] == fontId) {
                    currentIndex = i;
                    break;
                }
            }
            const char* currentPreview = fontIds.empty() ? "" : fontIds[currentIndex].c_str();
            if (ImGui::BeginCombo("Font ID", currentPreview)) {
                for (int i = 0; i < fontIds.size(); ++i) {
                    bool isSelected = (currentIndex == i);
                    if (ImGui::Selectable(fontIds[i].c_str(), isSelected)) {
                        std::string oldFontId = comp->GetFontId();
                        std::string newFontId = fontIds[i];
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldFontId, newFontId, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetFontId(v); }));
                    }
                    if (isSelected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        } else {
            char fontBuffer[128];
            strncpy_s(fontBuffer, sizeof(fontBuffer), fontId.c_str(), _TRUNCATE);
            static std::string startFont;
            if (ImGui::InputText("Font ID", fontBuffer, sizeof(fontBuffer))) {
                comp->SetFontId(fontBuffer);
            }
            if (ImGui::IsItemActivated()) startFont = fontId;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                std::string endFont = fontBuffer;
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                    startFont, endFont, [comp](const std::string& v){ comp->SetFontId(v); }));
            }
        }
        
        // Alignment
        TextAlignment align = comp->GetAlignment();
        const char* alignments[] = { "Left", "Center", "Right" };
        if (ImGui::BeginCombo("Alignment", alignments[static_cast<int>(align)])) {
            for (int i = 0; i < 3; ++i) {
                bool isSelected = (static_cast<int>(align) == i);
                if (ImGui::Selectable(alignments[i], isSelected)) {
                    TextAlignment oldAlign = comp->GetAlignment();
                    TextAlignment newAlign = static_cast<TextAlignment>(i);
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldAlign, newAlign, std::function<void(const TextAlignment&)>([comp](const TextAlignment& v){ comp->SetAlignment(v); }));
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Base Scale
        float scale = comp->GetBaseScale();
        if (ImGui::DragFloat("Base Scale", &scale, 0.1f, 0.1f, 1000.0f)) {
            comp->SetBaseScale(scale);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &scale, std::function<void(const float&)>([comp](const float& v){ comp->SetBaseScale(v); }));

        // Color
        Vector4 color = comp->GetColor();
        if (ImGui::ColorEdit4("Color", &color.x)) {
            comp->SetColor(color);
        }
        ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &color, std::function<void(const Vector4&)>([comp](const Vector4& v){ comp->SetColor(v); }));

        // TopMost
        bool isTopMost = comp->IsTopMost();
        if (ImGui::Checkbox("TopMost", &isTopMost)) {
            ComponentUIHelpers::PushInstantUndo(actionManager, comp->IsTopMost(), isTopMost, std::function<void(const bool&)>([comp](const bool& v){ comp->SetTopMost(v); }));
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
