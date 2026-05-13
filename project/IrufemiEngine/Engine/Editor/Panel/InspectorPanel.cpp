#include "InspectorPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/SceneManager.h"
#include "Framework/BaseScene.h"
#include "Framework/GameObject.h"
#include "../Core/EditorTheme.h"
#include <cstring>

void InspectorPanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
}

void InspectorPanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Inspector");

    if (auto selected = editorManager_->GetSelectedObject()) {
        char nameBuffer[256];
        strncpy_s(nameBuffer, selected->GetName().c_str(), sizeof(nameBuffer) - 1);
        
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 150); // Deleteボタンのスペースを確保
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            selected->SetName(nameBuffer);
        }
        
        // --- オブジェクト削除ボタン（赤色で右端に配置） ---
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        EditorTheme::PushDangerButtonStyle();
        if (ImGui::Button("Delete", ImVec2(70, 0))) {
            auto* engine = editorManager_->GetEngine();
            if (engine && engine->GetSceneManager()) {
                if (auto* baseScene = dynamic_cast<BaseScene*>(engine->GetSceneManager()->GetCurrentScene())) {
                    baseScene->RemoveGameObject(selected);
                    editorManager_->ClearSelectedObject(); // 選択解除
                }
            }
        }
        EditorTheme::PopButtonStyle();
        // ------------------------------------------------

        ImGui::Separator();

        // 削除されていなければコンポーネントのUIを描画
        if (auto sel = editorManager_->GetSelectedObject()) {
            sel->OnInspectorGUI();
        }
    } else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
#endif // EditorMode
