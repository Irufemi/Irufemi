#include "EditorManager.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Framework/SceneManager.h"
#include "Framework/IScene.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Engine/Manager/CollisionManager.h"

// FontAwesome 用のヘッダーを含める
#include "../../EngineResources/FontAwesome/IconsFontAwesome6.h"

void EditorManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    
    // アプリケーションの実行ディレクトリ（project直下など）をルートとして初期化
    // アプリケーションの実行ディレクトリをプロジェクトルートとして設定
    projectRootPath_ = std::filesystem::current_path();
    currentProjectBrowserPath_ = projectRootPath_;

    // 検索フィルタの初期化
    projectBrowserFilter_ = std::make_unique<ImGuiTextFilter>();
}

void EditorManager::DrawProjectBrowserTree(const std::filesystem::path& path) {
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

void EditorManager::DrawEditorUI() {
    // 1. 全画面を覆う DockSpace の背景ウィンドウを作成
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    // 背景ウィンドウの装飾を全て消すフラグ
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | 
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Editor DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // DockSpace 機能の起動
    ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // メニューバー
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene (main_scene.json)")) {
                if (engine_ && engine_->GetSceneManager()) {
                    if (auto* baseScene = dynamic_cast<BaseScene*>(engine_->GetSceneManager()->GetCurrentScene())) {
                        baseScene->SaveScene("resources/scenes/main_scene.json");
                    }
                }
            }
            if (ImGui::MenuItem("Load Scene (main_scene.json)")) {
                if (engine_ && engine_->GetSceneManager()) {
                    if (auto* baseScene = dynamic_cast<BaseScene*>(engine_->GetSceneManager()->GetCurrentScene())) {
                        baseScene->LoadScene("resources/scenes/main_scene.json");
                        selectedObject_.reset(); // ロードしたら選択を解除
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // 終了処理（PostQuitMessage）
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 2. 各種エディタパネルの描画
    DrawSceneView();
    DrawHierarchy();
    DrawInspector();
    DrawProjectBrowser();

    ImGui::End(); // Editor DockSpace
}

void EditorManager::DrawSceneView() {
    ImGui::Begin("Scene");

    // デバッグ線の描画ON/OFF
    bool* drawCollider = CollisionManager::GetInstance().GetIsDrawDebugLinePtr();
    if (drawCollider) {
        ImGui::Checkbox("Draw Colliders", drawCollider);
    }

    // エンジンからメインの描画結果（RenderTexture）を取得して画像として表示
    if (engine_ && engine_->GetMainRenderTexture()) {
        auto mainTexture = engine_->GetMainRenderTexture();
        
        // パネルの大きさを取得して画像をフィットさせる
        ImVec2 size = ImGui::GetContentRegionAvail();
        // 画像アスペクト比を維持したい場合は別途計算が必要だが、今回はパネルいっぱいに描画
        ImGui::Image((ImTextureID)mainTexture->GetSrvHandleGPU().ptr, size);
    }

    ImGui::End();
}

void EditorManager::DrawHierarchy() {
    ImGui::Begin("Hierarchy");

    if (engine_ && engine_->GetSceneManager()) {
        auto* currentScene = engine_->GetSceneManager()->GetCurrentScene();
        auto* baseScene = dynamic_cast<BaseScene*>(currentScene);

        if (baseScene) {
            // 背景クリックなどで選択解除する機能（オプション）
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                selectedObject_.reset();
            }

            // 循環参照チェック用ラムダ
            auto IsDescendant = [](std::shared_ptr<GameObject> potentialDescendant, GameObject* ancestor) {
                if (!potentialDescendant || !ancestor) return false;
                auto current = potentialDescendant->GetParent();
                while (current) {
                    if (current.get() == ancestor) return true;
                    current = current->GetParent();
                }
                return false;
            };

            // 再帰描画用ラムダ関数
            std::function<void(std::shared_ptr<GameObject>)> DrawNode = [&](std::shared_ptr<GameObject> obj) {
                if (!obj) return;

                bool isSelected = false;
                if (auto selected = selectedObject_.lock()) {
                    isSelected = (selected == obj);
                }

                // 描画開始時点での子の有無を保存（途中で子が追加されても不整合を起こさないため）
                bool hasChildrenAtStart = !obj->GetChildren().empty();

                // 子がいればツリーノード、いなければリーフ
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hasChildrenAtStart) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                if (isSelected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                // 識別用にポインタアドレスを使う
                bool isOpen = ImGui::TreeNodeEx((void*)obj.get(), flags, "%s", obj->GetName().c_str());

                // クリックで選択
                if (ImGui::IsItemClicked()) {
                    selectedObject_ = obj;
                }

                // --- Drag and Drop Source ---
                if (ImGui::BeginDragDropSource()) {
                    GameObject* ptr = obj.get();
                    ImGui::SetDragDropPayload("GAMEOBJECT", &ptr, sizeof(GameObject*));
                    ImGui::Text("Move %s", obj->GetName().c_str());
                    ImGui::EndDragDropSource();
                }

                // --- Drag and Drop Target ---
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
                        GameObject* payload_ptr = *(GameObject**)payload->Data;
                        
                        // 自分自身にはDropできない
                        // また、ドロップされるオブジェクトが「今の自分の親（先祖）」であってはならない（循環参照の防止）
                        if (payload_ptr != obj.get() && !IsDescendant(obj, payload_ptr)) {
                            if (auto dropObj = baseScene->FindGameObject(payload_ptr)) {
                                dropObj->SetParent(obj);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // コンテキストメニュー (右クリック)
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::Selectable("Duplicate")) {
                        auto clone = obj->Clone();
                        baseScene->AddGameObject(clone); // BaseScene の全体リストに追加
                        if (auto parent = obj->GetParent()) {
                            clone->SetParent(parent);
                        }
                    }
                    if (ImGui::Selectable("Delete")) {
                        if (auto parent = obj->GetParent()) {
                            parent->RemoveChild(obj);
                        }
                        baseScene->RemoveGameObject(obj);
                        if (auto selected = selectedObject_.lock()) {
                            if (selected == obj) selectedObject_.reset();
                        }
                    }
                    ImGui::EndPopup();
                }

                // 子ノードの描画
                if (isOpen && hasChildrenAtStart) {
                    // vector のコピーを回す（描画中に要素が削除・追加されても安全なように）
                    auto childrenCopy = obj->GetChildren();
                    for (auto& child : childrenCopy) {
                        DrawNode(child);
                    }
                    ImGui::TreePop();
                }
            };

            const auto& gameObjects = baseScene->GetGameObjects();
            auto gameObjectsCopy = gameObjects; // 描画中のリスト変更対策
            for (auto& obj : gameObjectsCopy) {
                // ルートオブジェクトのみを描画開始（子は再帰的に呼ばれる）
                if (obj && !obj->GetParent()) {
                    DrawNode(obj);
                }
            }

            // --- 余白でのD&D（ルートへ移動 ＆ アセット配置） ---
            ImGui::InvisibleButton("HierarchyDropZone", ImGui::GetContentRegionAvail());
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
                    GameObject* payload_ptr = *(GameObject**)payload->Data;
                    if (auto obj = baseScene->FindGameObject(payload_ptr)) {
                        obj->SetParent(nullptr); // 親を解除してルートに
                    }
                }
                // --- アセットのドロップを受け付ける ---
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_PATH")) {
                    std::string droppedPathStr = static_cast<const char*>(payload->Data);
                    std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                    std::string ext = droppedPath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    std::shared_ptr<GameObject> newObj = nullptr;
                    std::string stemString = reinterpret_cast<const char*>(droppedPath.stem().u8string().c_str());

                    if (ext == ".png" || ext == ".jpg" || ext == ".dds" || ext == ".bmp") {
                        newObj = std::make_shared<GameObject>("Sprite_" + stemString);
                        newObj->AddComponent<TransformComponent>();
                        auto spriteRenderer = newObj->AddComponent<SpriteRendererComponent>();
                        spriteRenderer->SetTexture(droppedPathStr); 
                        newObj->Initialize();
                    } else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".bin") {
                        newObj = std::make_shared<GameObject>("Model_" + stemString);
                        newObj->AddComponent<TransformComponent>();
                        auto meshRenderer = newObj->AddComponent<MeshRendererComponent>();
                        
                        // エンジンのObjClass等に合わせて、ファイル名のみを渡す
                        std::string modelName = reinterpret_cast<const char*>(droppedPath.filename().u8string().c_str());
                        meshRenderer->LoadModel(modelName); 
                        newObj->Initialize();
                    }

                    if (newObj) {
                        baseScene->AddGameObject(newObj);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // --- 全体の空白での右クリック「Create」メニューを表示 ---
            if (ImGui::BeginPopupContextItem("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
                if (ImGui::Selectable("Create Empty")) {
                    auto obj = std::make_shared<GameObject>("Empty Object");
                    obj->AddComponent<TransformComponent>();
                    obj->Initialize();
                    baseScene->AddGameObject(obj);
                }
                
                if (ImGui::BeginMenu("3D Object")) {
                    const char* shapes[] = { "Cube", "Sphere", "Cylinder", "Plane" };
                    PrimitiveType types[] = { PrimitiveType::Cube, PrimitiveType::Sphere, PrimitiveType::Cylinder, PrimitiveType::Plane };
                    
                    for (int i = 0; i < 4; ++i) {
                        if (ImGui::Selectable(shapes[i])) {
                            auto obj = std::make_shared<GameObject>(shapes[i]);
                            obj->AddComponent<TransformComponent>();
                            auto renderer = obj->AddComponent<PrimitiveRendererComponent>();
                            renderer->SetShape(types[i]);
                            obj->Initialize();
                            baseScene->AddGameObject(obj);
                        }
                    }
                    
                    ImGui::Separator();
                    if (ImGui::Selectable("Model (MeshRenderer)")) {
                        auto obj = std::make_shared<GameObject>("Model");
                        obj->AddComponent<TransformComponent>();
                        obj->AddComponent<MeshRendererComponent>();
                        obj->Initialize();
                        baseScene->AddGameObject(obj);
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("2D Object")) {
                    if (ImGui::Selectable("Sprite")) {
                        auto obj = std::make_shared<GameObject>("Sprite");
                        obj->AddComponent<TransformComponent>();
                        auto spriteRenderer = obj->AddComponent<SpriteRendererComponent>();
                        obj->GetComponent<TransformComponent>()->position_ = { 640.0f, 360.0f, 0.0f };
                        obj->Initialize();
                        baseScene->AddGameObject(obj);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            // --- Deleteキーによる削除対応 ---
            if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                if (auto selected = selectedObject_.lock()) {
                    if (auto parent = selected->GetParent()) {
                        parent->RemoveChild(selected);
                    }
                    baseScene->RemoveGameObject(selected);
                    selectedObject_.reset();
                }
            }
        }
    }

    ImGui::End();
}

void EditorManager::DrawInspector() {
    ImGui::Begin("Inspector");

    if (auto selected = selectedObject_.lock()) {
        char nameBuffer[256];
        strncpy_s(nameBuffer, selected->GetName().c_str(), sizeof(nameBuffer) - 1);
        
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 150); // Deleteボタンのスペースを確保
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            selected->SetName(nameBuffer);
        }
        
        // --- オブジェクト削除ボタン（赤色で右端に配置） ---
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // 赤色
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        if (ImGui::Button("Delete", ImVec2(70, 0))) {
            if (engine_ && engine_->GetSceneManager()) {
                if (auto* baseScene = dynamic_cast<BaseScene*>(engine_->GetSceneManager()->GetCurrentScene())) {
                    baseScene->RemoveGameObject(selected);
                    selectedObject_.reset(); // 選択解除
                }
            }
        }
        ImGui::PopStyleColor(3);
        // ------------------------------------------------

        ImGui::Separator();

        // 削除されていなければコンポーネントのUIを描画
        if (auto sel = selectedObject_.lock()) {
            sel->OnInspectorGUI();
        }
    } else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}

void EditorManager::DrawProjectBrowser() {
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
                    } else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".bin" || ext == ".mtl") {
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
                    ImGui::SetDragDropPayload("DND_ASSET_PATH", payloadPath.c_str(), payloadPath.length() + 1);
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
