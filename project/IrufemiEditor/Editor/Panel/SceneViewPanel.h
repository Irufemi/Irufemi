#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"
#include "Engine/Core/Math/Vector3.h"
#include <imgui.h>
#include "imgui/ImGuizmo.h"
#include "Engine/Graphics/Camera/OrbitCameraController.h"

/**
 * @class SceneViewPanel
 * @brief エディタのSceneビューを描画するパネル
 */
class SceneViewPanel : public IEditorPanel {
public:
    void Initialize(EditorManager* editorManager) override;
    void Draw() override;

private:
    EditorManager* editorManager_ = nullptr;
    OrbitCameraController cameraController_;

    // --- ギズモ用状態 ---
    ImGuizmo::OPERATION currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentGizmoMode_ = ImGuizmo::LOCAL;

    // --- Undo/Redo用の状態保存 ---
    bool wasUsingGizmo_ = false;
    Irufemi::Vector3 gizmoStartPos_;
    Irufemi::Vector3 gizmoStartRot_;
    Irufemi::Vector3 gizmoStartScale_;
    Irufemi::Vector3 gizmoStartColliderOffset_;
    Irufemi::Vector3 gizmoStartColliderSize_;

    // --- 内部ヘルパーメソッド ---
    void DrawToolbar(ImVec2 minPos, ImVec2 maxPos);
    void DrawImGuizmo(ImVec2 minPos, ImVec2 size);
    void HandleDragAndDrop(ImVec2 minPos, ImVec2 size);
    void HandlePicking(ImVec2 mousePos, ImVec2 minPos, ImVec2 maxPos, ImVec2 size);
};

#endif // EditorMode
