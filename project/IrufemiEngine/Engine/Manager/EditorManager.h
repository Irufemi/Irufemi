#pragma once

#ifdef EditorMode

class IrufemiEngine;

/**
 * @class EditorManager
 * @brief エディタのUIレイアウト（DockSpace、SceneViewなど）を統括するマネージャ
 */
class EditorManager {
public:
    void Initialize(IrufemiEngine* engine);
    void DrawEditorUI();

private:
    void DrawSceneView();

    IrufemiEngine* engine_ = nullptr;
};

#endif // EditorMode
