#include "EditorManager.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Framework/SceneManager.h"
#include "Framework/IScene.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/TransformComponent.h"
#include "Framework/PrimitiveRendererComponent.h"
#include "Framework/MeshRendererComponent.h"
#include "Framework/SpriteRendererComponent.h"

void EditorManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
    
    // アプリケーションの実行ディレクトリ（project直下など）をルートとして初期化
    currentProjectBrowserPath_ = std::filesystem::current_path();
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

            // --- 余白でのD&D（ルートへ移動） ---
            ImGui::InvisibleButton("HierarchyDropZone", ImGui::GetContentRegionAvail());
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
                    GameObject* payload_ptr = *(GameObject**)payload->Data;
                    if (auto obj = baseScene->FindGameObject(payload_ptr)) {
                        obj->SetParent(nullptr); // 親を解除してルートに
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

    if (std::filesystem::exists(currentProjectBrowserPath_) && std::filesystem::is_directory(currentProjectBrowserPath_)) {
        for (const auto& entry : std::filesystem::directory_iterator(currentProjectBrowserPath_)) {
            const auto& path = entry.path();
            // Windows環境などで日本語パスが文字化けしないよう、UTF-8に変換して取得する
            std::string filenameString = reinterpret_cast<const char*>(path.filename().u8string().c_str());
            
            // フォルダの場合
            if (entry.is_directory()) {
                // ダブルクリックでディレクトリを移動
                if (ImGui::Selectable(("[Folder] " + filenameString).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        currentProjectBrowserPath_ = path;
                        break; // イテレータの無効化を防ぐためループを抜ける
                    }
                }
            } 
            // ファイルの場合
            else {
                ImGui::Selectable(("[File] " + filenameString).c_str());
            }
        }
    }

    ImGui::End();
}

#endif // EditorMode
