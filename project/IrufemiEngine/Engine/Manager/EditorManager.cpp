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

void EditorManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;
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
        if (currentScene) {
            const auto& gameObjects = currentScene->GetGameObjects();
            
            for (size_t i = 0; i < gameObjects.size(); ++i) {
                const auto& obj = gameObjects[i];
                if (!obj) continue;

                bool isSelected = false;
                if (auto selected = selectedObject_.lock()) {
                    isSelected = (selected == obj);
                }

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (isSelected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID(static_cast<int>(i)); // ループ内でIDをユニークにする

                ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", obj->GetName().c_str());
                if (ImGui::IsItemClicked()) {
                    selectedObject_ = obj;
                }

                // アイテムごとの右クリックメニュー (Delete等)
                if (ImGui::BeginPopupContextItem("ItemContext", ImGuiPopupFlags_MouseButtonRight)) {
                    if (ImGui::Selectable("Delete")) {
                        auto* baseScene = dynamic_cast<BaseScene*>(currentScene);
                        if (baseScene) {
                            baseScene->RemoveGameObject(obj);
                            // もし選択中のオブジェクトを削除したら、選択を解除する
                            if (auto selected = selectedObject_.lock()) {
                                if (selected == obj) {
                                    selectedObject_.reset();
                                }
                            }
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID(); // PushIDの解除
            }

            // --- Deleteキーによる削除対応 ---
            if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                if (auto selected = selectedObject_.lock()) {
                    auto* baseScene = dynamic_cast<BaseScene*>(currentScene);
                    if (baseScene) {
                        baseScene->RemoveGameObject(selected);
                        selectedObject_.reset();
                    }
                }
            }

            // --- 全体の空白での右クリック「Create」メニューを表示 ---
            // ウィンドウ内の「何もない場所」を右クリックしたときだけ開くようにフラグを正しく指定する
            if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
                auto* baseScene = dynamic_cast<BaseScene*>(currentScene);
                if (baseScene) {
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
                }
                ImGui::EndPopup();
            }
        }
    }

    ImGui::End();
}

void EditorManager::DrawInspector() {
    ImGui::Begin("Inspector");

    if (auto selected = selectedObject_.lock()) {
        ImGui::Text("Name: %s", selected->GetName().c_str());
        
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
#endif // EditorMode
