#pragma once

#ifdef EditorMode
#include "../IEditorPanel.h"
#include <filesystem>
#include <memory>
#include <string>

struct ImGuiTextFilter;

/**
 * @class ProjectBrowserPanel
 * @brief プロジェクト内のファイルやディレクトリを閲覧・操作するパネル
 */
class ProjectBrowserPanel : public IEditorPanel {
public:
    ProjectBrowserPanel();
    ~ProjectBrowserPanel() override;

    void Initialize(EditorManager* editorManager) override;
    void Draw() override;

private:
    void DrawProjectBrowserTree(const std::filesystem::path& path);

    EditorManager* editorManager_ = nullptr;

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
