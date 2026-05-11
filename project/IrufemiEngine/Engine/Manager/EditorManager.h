#pragma once

#ifdef EditorMode
#include <memory>
#include <filesystem>

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
    void DrawProjectBrowser();

    IrufemiEngine* engine_ = nullptr;
    std::weak_ptr<GameObject> selectedObject_;
    
    // Project Browser 用のパス管理
    std::filesystem::path currentProjectBrowserPath_;
};

#endif // EditorMode
