#pragma once

#ifdef EditorMode
#include <memory>
#include <filesystem>

struct ImGuiTextFilter;

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
    void DrawProjectBrowserTree(const std::filesystem::path& path);

    IrufemiEngine* engine_ = nullptr;
    std::weak_ptr<GameObject> selectedObject_;
    
    // Project Browser 用のパス管理
    std::filesystem::path projectRootPath_;
    std::filesystem::path currentProjectBrowserPath_;

    // Project Browser 用の検索フィルタ
    std::unique_ptr<ImGuiTextFilter> projectBrowserFilter_;

    // Project Browser 用のファイル操作状態
    std::filesystem::path renamingTarget_;
    char projectBrowserInputBuffer_[256] = "";
    bool isCreatingFolder_ = false;
};

#endif // EditorMode
