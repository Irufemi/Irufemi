#include "ConsolePanel.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "Engine/Core/Utility/Log.h"

void ConsolePanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
}

void ConsolePanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        Log::ClearLogHistory();
    }
    
    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    const auto& logHistory = Log::GetLogHistory();
    for (const auto& logEntry : logHistory) {
        ImGui::TextUnformatted(logEntry.c_str());
    }

    // 自動スクロール
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    ImGui::End();
}
#endif // EditorMode
