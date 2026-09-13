#pragma once

#ifdef EditorMode
#include "Core/IEditorPanel.h"

/**
 * @class InspectorPanel
 * @brief 選択されたGameObjectのコンポーネントを描画するパネル
 */
class InspectorPanel : public IEditorPanel {
public:
    void Initialize(EditorManager* editorManager) override;
    void Draw() override;
    const char* GetName() const override {
        return "Inspector";
    }

private:
    EditorManager* editorManager_ = nullptr;
};

#endif // EditorMode
