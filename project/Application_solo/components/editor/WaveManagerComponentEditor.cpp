#if defined(_DEBUG) || defined(EditorMode) || defined(DEVELOPMENT)
#include "WaveManagerComponentEditor.h"
#include "../WaveManagerComponent.h"
#include <imgui/imgui.h>
#include <string>
#include <cstring>

void WaveManagerComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto waveManager = dynamic_cast<WaveManagerComponent*>(component);
    if (!waveManager) return;

    ImGui::Text("WaveManager Settings");
    ImGui::Separator();

    std::string path = waveManager->GetLevelDataPath();
    char buffer[256];
    strncpy_s(buffer, sizeof(buffer), path.c_str(), _TRUNCATE);
    buffer[sizeof(buffer) - 1] = '\0';
    if (ImGui::InputText("Level Data Path", buffer, sizeof(buffer))) {
        waveManager->SetLevelDataPath(buffer);
    }

    ImGui::Spacing();
    if (ImGui::Button("Reload JSON Data", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        waveManager->ReloadLevelData();
    }

    ImGui::Separator();
    ImGui::Text("Loaded Events: %d", (int)waveManager->GetAllEvents().size());

    if (ImGui::TreeNode("Event List")) {
        int i = 0;
        for (const auto& ev : waveManager->GetAllEvents()) {
            std::string label = "Event " + std::to_string(i) + " [" + ev.eventType + "] Dist: " + std::to_string(ev.triggerDistance);
            if (ImGui::TreeNode((void*)(intptr_t)i, "%s", label.c_str())) {
                ImGui::Text("Type: %s", ev.eventType.c_str());
                ImGui::Text("Trigger Dist: %.1f", ev.triggerDistance);
                if (ev.parameters.contains("WaveId")) {
                    ImGui::Text("WaveId: %s", ev.parameters["WaveId"].get<std::string>().c_str());
                } else if (ev.parameters.contains("GroupId")) {
                    ImGui::Text("GroupId: %s", ev.parameters["GroupId"].get<std::string>().c_str());
                }
                if (ev.parameters.contains("Count")) {
                    ImGui::Text("Count: %d", ev.parameters["Count"].get<int>());
                }
                if (ev.parameters.contains("Formation")) {
                    ImGui::Text("Formation: %s", ev.parameters["Formation"].get<std::string>().c_str());
                }
                ImGui::TreePop();
            }
            i++;
        }
        ImGui::TreePop();
    }
}
#endif
