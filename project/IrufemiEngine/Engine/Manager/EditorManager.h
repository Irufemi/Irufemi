#pragma once

#ifdef EditorMode
#include <memory>
#include <vector>

class IrufemiEngine;
class GameObject;
class IEditorPanel;
class EditorActionManager;
class EditorShortcutManager;
class ComponentEditorRegistry;

/**
 * @class EditorManager
 * @brief エディタのUIレイアウト（DockSpace、SceneViewなど）を統括するマネージャ
 */
class EditorManager {
public:
    EditorManager();
    ~EditorManager();

    void Initialize(IrufemiEngine* engine);
    void Update();
    void DrawEditorUI();

    /** @name 各パネルからアクセスするための状態管理 Getter/Setter */
    ///@{
    IrufemiEngine* GetEngine() const { return engine_; }
    
    EditorActionManager* GetActionManager() const { return actionManager_.get(); }
    EditorShortcutManager* GetShortcutManager() const { return shortcutManager_.get(); }
    ComponentEditorRegistry* GetComponentEditorRegistry() const { return componentEditorRegistry_.get(); }
    
    std::shared_ptr<GameObject> GetSelectedObject() const { return selectedObject_.lock(); }
    void SetSelectedObject(std::shared_ptr<GameObject> obj) { selectedObject_ = obj; }
    void ClearSelectedObject() { selectedObject_.reset(); }
    ///@}

private:

    IrufemiEngine* engine_ = nullptr;
    std::weak_ptr<GameObject> selectedObject_;

    std::unique_ptr<EditorActionManager> actionManager_;
    std::unique_ptr<EditorShortcutManager> shortcutManager_;
    std::unique_ptr<ComponentEditorRegistry> componentEditorRegistry_;

    // 各エディタパネル
    std::vector<std::unique_ptr<IEditorPanel>> panels_;
};

#endif // EditorMode
