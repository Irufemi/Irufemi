#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "BossComponentEditor.h"
#include "../Boss/BossComponent.h"
#include <imgui/imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include "Engine/Core/Utility/Log.h"
#include <iostream>

void BossComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto comp = dynamic_cast<BossComponent*>(component);
    if (!comp) return;

    ImGui::Text("Boss Settings");
    ImGui::Separator();

    std::string path = comp->GetStatusDataPath();
    char buffer[256];
    strncpy_s(buffer, sizeof(buffer), path.c_str(), _TRUNCATE);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (ImGui::InputText("Status Data Path", buffer, sizeof(buffer))) {
        comp->SetStatusDataPath(buffer);
    }
    
    if (ImGui::Button("Reload JSON", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        comp->LoadStatusFromJson();
    }
    ImGui::Spacing();
    
    if (!path.empty()) {
        nlohmann::json j;
        std::ifstream file(path);
        if (file.is_open()) {
            file >> j;
            file.close();
            
            bool modified = false;
            
            if (ImGui::TreeNodeEx("Gameplay Data (Saved in JSON)", ImGuiTreeNodeFlags_DefaultOpen)) {
                
                float maxHp = j.value("maxHp", 1000.0f);
                if (ImGui::DragFloat("Max HP", &maxHp, 10.0f, 1.0f, 100000.0f)) {
                    j["maxHp"] = maxHp;
                    modified = true;
                }
                
                int maxShieldCount = j.value("maxShieldCount", 100);
                if (ImGui::DragInt("Max Shield Count", &maxShieldCount, 1, 0, 500)) {
                    j["maxShieldCount"] = maxShieldCount;
                    modified = true;
                }
                
                float shieldRadius = j.value("shieldRadius", 8.0f);
                if (ImGui::DragFloat("Shield Radius", &shieldRadius, 0.1f, 1.0f, 50.0f)) {
                    j["shieldRadius"] = shieldRadius;
                    modified = true;
                }
                
                float beamInterval = j.value("beamInterval", 10.0f);
                if (ImGui::DragFloat("Beam Interval", &beamInterval, 0.1f, 0.1f, 60.0f)) {
                    j["beamInterval"] = beamInterval;
                    modified = true;
                }
                
                float beamRange = j.value("beamRange", 1000.0f);
                if (ImGui::DragFloat("Beam Range", &beamRange, 10.0f, 10.0f, 10000.0f)) {
                    j["beamRange"] = beamRange;
                    modified = true;
                }
                
                ImGui::TreePop();
            }
            
            if (modified) {
                std::ofstream outFile(path);
                if (outFile.is_open()) {
                    outFile << j.dump(4);
                    outFile.close();
                    
                    comp->LoadStatusFromJson();
                } else {
                    Log::OutPutLog(std::cout, "[Editor] Failed to save JSON: " + path + "\n");
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "JSON File Not Found!");
        }
    }
}
#endif
