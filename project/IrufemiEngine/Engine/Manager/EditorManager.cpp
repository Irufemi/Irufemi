#include "EditorManager.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Framework/SceneManager.h"
#include "Framework/IScene.h"
#include "Framework/GameObject.h"

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

                ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s", obj->GetName().c_str());
                if (ImGui::IsItemClicked()) {
                    selectedObject_ = obj;
                }
            }
        }
    }

    ImGui::End();
}

void EditorManager::DrawInspector() {
    ImGui::Begin("Inspector");

    if (auto selected = selectedObject_.lock()) {
        ImGui::Text("Name: %s", selected->GetName().c_str());
        ImGui::Separator();

        selected->OnInspectorGUI();
    } else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
#endif // EditorMode
