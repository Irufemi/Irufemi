#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"
#include "Engine/Core/Math/Vector3.h"

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

    // --- エディタカメラ用状態 ---
    bool isCameraInitialized_ = false;
    Vector3 cameraTarget_ = {0.0f, 0.0f, 0.0f};
    float cameraDistance_ = 50.0f;
};

#endif // EditorMode
