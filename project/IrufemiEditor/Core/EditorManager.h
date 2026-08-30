#pragma once

#ifdef EditorMode
#include "Core/System/IEngineExtension.h"
#include <memory>
#include <string>
#include <vector>

class IrufemiEngine;
class GameObject;
class IEditorPanel;
class EditorActionManager;
class EditorShortcutManager;
class ComponentEditorRegistry;

/**
 * @brief エディタの現在の動作モード
 */
enum class EditorModeState { Edit, Playing, Paused, PrefabEdit };

/**
 * @class EditorManager
 * @brief エディタのUIレイアウト（DockSpace、SceneViewなど）を統括するマネージャ
 */
class EditorManager : public IEngineExtension {
public:
    EditorManager();
    ~EditorManager() override;

    void OnInitialize(IrufemiEngine* engine) override;
    void OnUpdate(float deltaTime) override;
    void OnDrawUI() override;

    /** @name 各パネルからアクセスするための状態管理 Getter/Setter */
    ///@{
    IrufemiEngine* GetEngine() const {
        return engine_;
    }

    EditorActionManager* GetActionManager() const {
        return actionManager_.get();
    }
    EditorShortcutManager* GetShortcutManager() const {
        return shortcutManager_.get();
    }
    ComponentEditorRegistry* GetComponentEditorRegistry() const {
        return componentEditorRegistry_.get();
    }

    std::shared_ptr<GameObject> GetSelectedObject() const;
    void SetSelectedObject(std::shared_ptr<GameObject> obj);
    void ClearSelectedObject();

    EditorModeState GetCurrentMode() const {
        return currentMode_;
    }
    bool IsPlayMode() const {
        return currentMode_ != EditorModeState::Edit;
    }
    ///@}

    /**
     * @brief 編集モードからプレイモードへ移行し、現在のシーン状態を一時保存する
     */
    void EnterPlayMode();

    /**
     * @brief プレイモードから編集モードへ戻り、シーン状態を一時保存から復元する
     */
    void ExitPlayMode();

    /**
     * @brief プレイ中に一時停止/再開を切り替える
     */
    void TogglePauseMode();

    /**
     * @brief Prefabモードに入る
     */
    void EnterPrefabMode(const std::string& prefabPath);

    /**
     * @brief Prefabモードから出る
     * @param saveChanges trueなら現在のルートオブジェクトをPrefabとして上書き保存する
     */
    void ExitPrefabMode(bool saveChanges);

private:
    IrufemiEngine* engine_ = nullptr;

    EditorModeState currentMode_ = EditorModeState::Edit;
    std::string playModeStartSceneName_ = "";
    std::string editingPrefabPath_ = "";
    bool isStepRequested_ = false; // コマ送りの予約フラグ

    // レイアウトのリセット用フラグ
    bool resetLayout_ = false;

    std::unique_ptr<EditorActionManager> actionManager_;
    std::unique_ptr<EditorShortcutManager> shortcutManager_;
    std::unique_ptr<ComponentEditorRegistry> componentEditorRegistry_;

    // 各エディタパネル
    std::vector<std::unique_ptr<IEditorPanel>> panels_;
};

#endif // EditorMode
