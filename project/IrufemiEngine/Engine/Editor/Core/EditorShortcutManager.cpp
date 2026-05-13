#include "EditorShortcutManager.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include "Engine/Manager/EditorManager.h"
#include "EditorActionManager.h"

EditorShortcutManager::EditorShortcutManager(EditorManager* editor, EditorActionManager* actionManager)
    : editorManager_(editor), actionManager_(actionManager) {}

void EditorShortcutManager::Update() {
    if (!editorManager_ || !actionManager_) return;

    // もしImGuiがテキスト入力中ならショートカットを無視する
    if (ImGui::GetIO().WantTextInput) return;

    // Deleteキーでの削除
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (auto selected = editorManager_->GetSelectedObject()) {
            actionManager_->DeleteObject(selected);
        }
    }

    // Ctrl + D での複製
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        if (auto selected = editorManager_->GetSelectedObject()) {
            actionManager_->DuplicateObject(selected);
        }
    }
}
#endif // EditorMode
