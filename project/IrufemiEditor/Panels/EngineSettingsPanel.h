#pragma once

#ifdef EditorMode
#include "Core/IEditorPanel.h"

/**
 * @class EngineSettingsPanel
 * @brief エディタ上でエンジンの環境設定（ポストプロセス、ライティング、カメラ、ディスプレイ等）を編集するパネル
 */
class EngineSettingsPanel : public IEditorPanel {
public:
    EngineSettingsPanel() = default;
    ~EngineSettingsPanel() override = default;

    void Initialize(EditorManager* editorManager) override;
    void Draw() override;

    const char* GetName() const override {
        return "Engine Settings";
    }

private:
    EditorManager* editorManager_ = nullptr;
};

#endif // EditorMode
