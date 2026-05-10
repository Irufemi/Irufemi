#pragma once

#ifdef EditorMode
#include <memory>

class IrufemiEngine;
class GameObject;

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
    void DrawHierarchy();
    void DrawInspector();

    IrufemiEngine* engine_ = nullptr;
    std::weak_ptr<GameObject> selectedObject_;
};

#endif // EditorMode
