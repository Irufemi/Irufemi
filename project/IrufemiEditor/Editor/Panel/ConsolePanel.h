#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"

/**
 * @class ConsolePanel
 * @brief エンジンのログ出力（Log::OutPutLog）をキャッチし、エディタ上に一覧表示するパネル
 */
class ConsolePanel : public IEditorPanel {
public:
    void Initialize(EditorManager* editorManager) override;
    void Draw() override;

private:
    EditorManager* editorManager_ = nullptr;
};
#endif // EditorMode
