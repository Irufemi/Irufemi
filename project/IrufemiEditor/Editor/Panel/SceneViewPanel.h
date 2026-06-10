#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"
#include "Engine/Core/Math/Vector3.h"
#include <imgui.h>
#include "imgui/ImGuizmo.h"
#include "Editor/Utils/EditorCameraController.h"

/**
 * @class SceneViewPanel
 * @brief 繧ｨ繝・ぅ繧ｿ縺ｮScene繝薙Η繝ｼ繧呈緒逕ｻ縺吶ｋ繝代ロ繝ｫ
 */
class SceneViewPanel : public IEditorPanel {
public:
    void Initialize(EditorManager* editorManager) override;
    void Draw() override;

private:
    EditorManager* editorManager_ = nullptr;
    EditorCameraController cameraController_;

    // --- 繧ｮ繧ｺ繝｢逕ｨ迥ｶ諷・---
    ImGuizmo::OPERATION currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode_ = ImGuizmo::LOCAL;

    // --- 蜀・Κ繝倥Ν繝代・繝｡繧ｽ繝・ラ ---
    void DrawImGuizmo(ImVec2 minPos, ImVec2 size);
    void HandleDragAndDrop();
    void HandlePicking(ImVec2 mousePos, ImVec2 minPos, ImVec2 maxPos, ImVec2 size);
};

#endif // EditorMode
