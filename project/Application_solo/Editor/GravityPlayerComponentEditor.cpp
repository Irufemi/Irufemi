#ifdef EditorMode
#include "Editor/GravityPlayerComponentEditor.h"
#include "Player/GravityPlayerComponent.h"
#include <imgui/imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include "Core/Utility/Log.h"
#include <iostream>

void GravityPlayerComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto comp = dynamic_cast<GravityPlayerComponent*>(component);
    if (!comp) {
        return;
    }

    ImGui::Text("Gravity Player Settings");
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

    // JSONファイルから直接ロードして編集・保存する
    if (!path.empty()) {
        nlohmann::json j;
        std::ifstream file(path);
        if (file.is_open()) {
            file >> j;
            file.close();

            bool modified = false;

            if (ImGui::TreeNodeEx("Gameplay Data (Saved in JSON)", ImGuiTreeNodeFlags_DefaultOpen)) {

                int maxHp = j.value("maxHp", 100);
                if (ImGui::DragInt("Max HP", &maxHp, 1, 1, 10000)) {
                    j["maxHp"] = maxHp;
                    modified = true;
                }

                int maxOrbitCount = j.value("maxOrbitCount", 5);
                if (ImGui::DragInt("Max Orbit Count", &maxOrbitCount, 1, 1, 50)) {
                    j["maxOrbitCount"] = maxOrbitCount;
                    modified = true;
                }

                float pullRadius = j.value("pullRadius", 100.0f);
                if (ImGui::DragFloat("Pull Radius", &pullRadius, 1.0f, 1.0f, 1000.0f)) {
                    j["pullRadius"] = pullRadius;
                    modified = true;
                }

                float throwInterval = j.value("throwInterval", 0.15f);
                if (ImGui::DragFloat("Throw Interval", &throwInterval, 0.01f, 0.01f, 5.0f)) {
                    j["throwInterval"] = throwInterval;
                    modified = true;
                }

                float orbitRadiusMin = j.value("orbitRadiusMin", 2.0f);
                if (ImGui::DragFloat("Orbit Radius Min", &orbitRadiusMin, 0.1f, 0.1f, 20.0f)) {
                    j["orbitRadiusMin"] = orbitRadiusMin;
                    modified = true;
                }

                float orbitRadiusMax = j.value("orbitRadiusMax", 4.0f);
                if (ImGui::DragFloat("Orbit Radius Max", &orbitRadiusMax, 0.1f, 0.1f, 20.0f)) {
                    j["orbitRadiusMax"] = orbitRadiusMax;
                    modified = true;
                }

                ImGui::TreePop();
            }

            // ファイルへの上書き保存とコンポーネントへの反映
            if (modified) {
                std::ofstream outFile(path);
                if (outFile.is_open()) {
                    outFile << j.dump(4);
                    outFile.close();

                    // コンポーネント側も即時反映
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
