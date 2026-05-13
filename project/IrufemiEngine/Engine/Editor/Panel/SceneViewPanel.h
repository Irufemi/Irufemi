#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"

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
};

#endif // EditorMode
