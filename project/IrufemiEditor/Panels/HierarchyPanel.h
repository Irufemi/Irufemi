#pragma once

#ifdef EditorMode
#include "Core/IEditorPanel.h"

/**
 * @class HierarchyPanel
 * @brief シーン内のGameObjectの階層構造を描画・編集するパネル
 */
class HierarchyPanel : public IEditorPanel {
public:
    void Initialize(EditorManager* editorManager) override;
    void Draw() override;
    const char* GetName() const override {
        return "Hierarchy";
    }

private:
    EditorManager* editorManager_ = nullptr;
};

#endif // EditorMode
