#include "ProjectBrowserPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "../../../EngineResources/FontAwesome/IconsFontAwesome6.h"
#include "../Core/EditorDragDrop.h"
#include <algorithm>

ProjectBrowserPanel::ProjectBrowserPanel() {
    projectBrowserFilter_ = std::make_unique<ImGuiTextFilter>();
}

ProjectBrowserPanel::~ProjectBrowserPanel() = default;

void ProjectBrowserPanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
    
    // アプリケーションの実行ディレクトリ（project直下など）をルートとして初期化
    projectRootPath_ = std::filesystem::current_path();
    currentProjectBrowserPath_ = projectRootPath_;
}

void ProjectBrowserPanel::DrawProjectBrowserTree(const std::filesystem::path& path) {
    std::string folderName = reinterpret_cast<const char*>(path.filename().u8string().c_str());
    if (folderName.empty()) folderName = "Root";

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    // 現在選択されているフォルダならハイライト
    if (path == currentProjectBrowserPath_) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // 中身にフォルダがあるかチェック
    bool hasSubDirectories = false;
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                hasSubDirectories = true;
                break;
            }
        }
    }

    if (!hasSubDirectories) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    // フォルダ名でツリーノードを描画
    std::string pathId = path.string();
    std::string treeLabel = std::string(ICON_FA_FOLDER) + " " + folderName;
    bool isOpen = ImGui::TreeNodeEx(pathId.c_str(), flags, "%s", treeLabel.c_str());

    // クリックされたら右ペインの表示を切り替え
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        currentProjectBrowserPath_ = path;
    }

    if (isOpen && hasSubDirectories) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                DrawProjectBrowserTree(entry.path());
            }
        }
        ImGui::TreePop();
    }
}

void ProjectBrowserPanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Project");

    // 上部に「上へ」戻るボタンと現在のパスを表示
    if (ImGui::Button("Up") && currentProjectBrowserPath_.has_parent_path()) {
        currentProjectBrowserPath_ = currentProjectBrowserPath_.parent_path();
    }
    ImGui::SameLine();
    
    // パスもUTF-8に変換して表示
    std::string currentPathStr = reinterpret_cast<const char*>(currentProjectBrowserPath_.u8string().c_str());
    ImGui::Text("%s", currentPathStr.c_str());
    ImGui::Separator();

    // 検索フィルタの描画
    projectBrowserFilter_->Draw("Search", ImGui::GetContentRegionAvail().x);
    ImGui::Separator();

    // --- 左右にペインを分割 (ImGui::Table) ---
    if (ImGui::BeginTable("ProjectBrowserTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
        
        // --- 左ペイン（フォルダツリー） ---
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        
        ImGui::BeginChild("ProjectTreePane", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        DrawProjectBrowserTree(projectRootPath_);
        ImGui::EndChild();

        // --- 右ペイン（フォルダの中身） ---
        ImGui::TableSetColumnIndex(1);
        
        ImGui::BeginChild("ProjectContentPane", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (std::filesystem::exists(currentProjectBrowserPath_) && std::filesystem::is_directory(currentProjectBrowserPath_)) {
            float itemWidth = 80.0f;
            float itemHeight = 90.0f;
            float windowVisibleX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

            for (const auto& entry : std::filesystem::directory_iterator(currentProjectBrowserPath_)) {
                const auto& path = entry.path();
                // Windows環境などで日本語パスが文字化けしないよう、UTF-8に変換して取得する
                std::string filenameString = reinterpret_cast<const char*>(path.filename().u8string().c_str());

                // フィルタリング（マッチしなければスキップ）
                if (!projectBrowserFilter_->PassFilter(filenameString.c_str())) {
                    continue;
                }

                // 拡張子による色とアイコンの判定
                ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // デフォルト白
                std::string icon = ICON_FA_FILE;

                if (entry.is_directory()) {
                    icon = ICON_FA_FOLDER;
                    textColor = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // 黄色
                } else {
                    std::string ext = path.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower); // 小文字化

                    if (ext == ".png" || ext == ".jpg" || ext == ".dds" || ext == ".bmp") {
                        icon = ICON_FA_IMAGE;
                        textColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // 水色
                    } else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".glb" || ext == ".mtl") {
                        icon = ICON_FA_CUBES;
                        textColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // 緑色
                    } else if (ext == ".json") {
                        icon = ICON_FA_MAP;
                        textColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); // オレンジ色
                    } else if (ext == ".wav" || ext == ".mp3") {
                        icon = ICON_FA_MUSIC;
                        textColor = ImVec4(1.0f, 0.4f, 0.8f, 1.0f); // ピンク色
                    }
                }

                ImGui::PushID(path.string().c_str());
                ImGui::BeginGroup();

                bool isRenamingThis = (renamingTarget_ == path);

                // --- タイルの下地となるSelectable ---
                bool isDoubleClick = false;
                if (ImGui::Selectable("##Tile", false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(itemWidth, itemHeight))) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        isDoubleClick = true;
                    }
                }

                ImVec2 itemMin = ImGui::GetItemRectMin();

                // フォルダのダブルクリックによる移動
                if (isDoubleClick && entry.is_directory()) {
                    currentProjectBrowserPath_ = path;
                    ImGui::EndGroup();
                    ImGui::PopID();
                    break; // イテレータ無効化を防ぐため抜ける
                }

                // --- 右クリックメニュー ---
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Rename")) {
                        renamingTarget_ = path;
                        strncpy_s(projectBrowserInputBuffer_, sizeof(projectBrowserInputBuffer_), filenameString.c_str(), _TRUNCATE);
                    }
                    if (ImGui::MenuItem("Delete")) {
                        try {
                            std::filesystem::remove_all(path);
                        } catch (...) {}
                    }
                    ImGui::EndPopup();
                }

                // --- ドラッグ＆ドロップソース (ファイルのみ) ---
                if (!entry.is_directory() && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    std::string payloadPath;
                    try {
                        payloadPath = reinterpret_cast<const char*>(std::filesystem::relative(path, std::filesystem::current_path()).u8string().c_str());
                        std::replace(payloadPath.begin(), payloadPath.end(), '\\', '/');
                    } catch (...) {
                        payloadPath = reinterpret_cast<const char*>(path.u8string().c_str());
                    }
                    ImGui::SetDragDropPayload(EditorDragDrop::PayloadAssetPath, payloadPath.c_str(), payloadPath.length() + 1);
                    ImGui::Text("Place Asset: %s", filenameString.c_str());
                    ImGui::EndDragDropSource();
                }

                // --- 見た目の描画 (アイコンとファイル名) ---
                if (isRenamingThis) {
                    ImGui::SetCursorScreenPos(ImVec2(itemMin.x + 4.0f, itemMin.y + 40.0f));
                    ImGui::SetKeyboardFocusHere();
                    ImGui::PushItemWidth(itemWidth - 8.0f);
                    if (ImGui::InputText("##rename", projectBrowserInputBuffer_, sizeof(projectBrowserInputBuffer_), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        try {
                            std::string newNameString = projectBrowserInputBuffer_;
                            if (!newNameString.empty()) {
                                std::filesystem::path newPath = path.parent_path() / newNameString;
                                std::filesystem::rename(path, newPath);
                            }
                        } catch (...) {}
                        renamingTarget_.clear();
                    }
                    ImGui::PopItemWidth();
                    
                    if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                        renamingTarget_.clear();
                    }
                } else {
                    // 中央にアイコンを描画
                    ImVec2 iconSize = ImGui::CalcTextSize(icon.c_str());
                    ImGui::GetWindowDrawList()->AddText(ImVec2(itemMin.x + (itemWidth - iconSize.x) * 0.5f, itemMin.y + 20.0f), ImGui::GetColorU32(textColor), icon.c_str());

                    // 下部にファイル名を描画 (切り詰め処理)
                    std::string displayName = filenameString;
                    if (displayName.length() > 10) {
                        displayName = displayName.substr(0, 8) + "..";
                    }
                    ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
                    ImGui::GetWindowDrawList()->AddText(ImVec2(itemMin.x + (itemWidth - textSize.x) * 0.5f, itemMin.y + 50.0f), ImGui::GetColorU32(ImGuiCol_Text), displayName.c_str());
                }

                ImGui::EndGroup();
                ImGui::PopID();

                // --- 折り返しの計算 ---
                float lastItemMaxX = ImGui::GetItemRectMax().x;
                float nextItemMaxX = lastItemMaxX + ImGui::GetStyle().ItemSpacing.x + itemWidth;
                if (nextItemMaxX < windowVisibleX) {
                    ImGui::SameLine();
                }
            }
        }

    // 新規フォルダ作成の入力フィールド
    if (isCreatingFolder_) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f)); // フォルダの黄色
        ImGui::Text("%s", ICON_FA_FOLDER);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetKeyboardFocusHere();
        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##newfolder", projectBrowserInputBuffer_, sizeof(projectBrowserInputBuffer_), ImGuiInputTextFlags_EnterReturnsTrue)) {
            try {
                std::string newNameString = projectBrowserInputBuffer_;
                if (!newNameString.empty()) {
                    std::filesystem::path newPath = currentProjectBrowserPath_ / newNameString;
                    std::filesystem::create_directory(newPath);
                }
            } catch (...) {}
            isCreatingFolder_ = false;
        }
        ImGui::PopItemWidth();

        if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
            isCreatingFolder_ = false;
        }
    }

    // ウィンドウ全体に対する右クリックメニュー（空白部分用）
    if (ImGui::BeginPopupContextWindow("ProjectBrowserContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Create Folder")) {
            isCreatingFolder_ = true;
            projectBrowserInputBuffer_[0] = '\0'; // バッファをクリア
        }
        ImGui::EndPopup();
    }

        ImGui::EndChild(); // End ProjectContentPane
        ImGui::EndTable(); // End ProjectBrowserTable
    } // End if (BeginTable)

    ImGui::End();
}
#endif // EditorMode
