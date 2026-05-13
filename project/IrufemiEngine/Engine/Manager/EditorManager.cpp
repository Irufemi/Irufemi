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

// 分離したエディタパネル群
#include "Engine/Editor/IEditorPanel.h"
#include "Engine/Editor/Panel/SceneViewPanel.h"
#include "Engine/Editor/Panel/HierarchyPanel.h"
#include "Engine/Editor/Panel/InspectorPanel.h"
#include "Engine/Editor/Panel/ProjectBrowserPanel.h"

// Editor Core
#include "Engine/Editor/Core/EditorActionManager.h"
#include "Engine/Editor/Core/EditorShortcutManager.h"

// FontAwesome 用のヘッダーを含める
#include "../../EngineResources/FontAwesome/IconsFontAwesome6.h"

EditorManager::EditorManager() = default;
EditorManager::~EditorManager() = default;

void EditorManager::Initialize(IrufemiEngine* engine) {
    engine_ = engine;

    actionManager_ = std::make_unique<EditorActionManager>(this);
    shortcutManager_ = std::make_unique<EditorShortcutManager>(this, actionManager_.get());

    // 各パネルの生成と初期化
    panels_.push_back(std::make_unique<SceneViewPanel>());
    panels_.push_back(std::make_unique<HierarchyPanel>());
    panels_.push_back(std::make_unique<InspectorPanel>());
    panels_.push_back(std::make_unique<ProjectBrowserPanel>());

    for (auto& panel : panels_) {
        panel->Initialize(this);
    }
}

void EditorManager::Update() {
    if (shortcutManager_) {
        shortcutManager_->Update();
    }
}

void EditorManager::DrawEditorUI() {
    if (!engine_ || !engine_->GetMainRenderTexture()) return;

    Update(); // ここでショートカット処理を呼ぶ

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
    for (auto& panel : panels_) {
        panel->Draw();
    }

    ImGui::End(); // Editor DockSpace
}

#endif // EditorMode
