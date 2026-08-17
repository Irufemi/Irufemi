#include "Core/EditorManager.h"

#ifdef EditorMode
#include <filesystem>
#include "imgui/imgui.h"
#include "Core/System/IrufemiEngine.h"
#include "Core/Utility/Log.h"
#include <iostream>
#include "RHI/DirectX12/RenderTexture.h"
#include "imgui/imgui_internal.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/IScene.h"
#include "Framework/GameObject/GameObject.h"
#include "Framework/Scene/BaseScene.h"
#include "Framework/Scene/SceneSerializer.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Physics/CollisionManager.h"
#include "Renderer/DrawManager.h"
#include "Renderer/Pipeline/RenderGraph/RenderGraph.h"

// 分離したエディタパネル群
#include "Core/IEditorPanel.h"
#include "Panels/SceneViewPanel.h"
#include "Panels/HierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ProjectBrowserPanel.h"
#include "Panels/ConsolePanel.h"

// Editor Core
#include "Commands/EditorActionManager.h"
#include "Commands/EditorShortcutManager.h"
#include "Inspectors/ComponentEditorRegistry.h"
#include "Core/EditorTheme.h"

// FontAwesome 用のヘッダーを含める
#include "EngineResources/FontAwesome/IconsFontAwesome6.h"

EditorManager::EditorManager() = default;
EditorManager::~EditorManager() = default;

void EditorManager::OnInitialize(IrufemiEngine* engine) {
    engine_ = engine;
    engine_->SetPlayMode(false); // 初期はEditモード

    actionManager_ = std::make_unique<EditorActionManager>(this);
    shortcutManager_ = std::make_unique<EditorShortcutManager>(this, actionManager_.get());

    componentEditorRegistry_ = std::make_unique<ComponentEditorRegistry>();
    componentEditorRegistry_->RegisterAllEditors();

    // テーマの適用
    EditorTheme::ApplyDarkTheme();

    // 各パネルの生成と初期化
    panels_.push_back(std::make_unique<SceneViewPanel>());
    panels_.push_back(std::make_unique<HierarchyPanel>());
    panels_.push_back(std::make_unique<InspectorPanel>());
    panels_.push_back(std::make_unique<ProjectBrowserPanel>());
    panels_.push_back(std::make_unique<ConsolePanel>());

    for (auto& panel : panels_) {
        panel->Initialize(this);
    }
}

std::shared_ptr<GameObject> EditorManager::GetSelectedObject() const {
    return engine_ ? engine_->GetSelectedObject() : nullptr;
}

void EditorManager::SetSelectedObject(std::shared_ptr<GameObject> obj) {
    if (engine_) engine_->SetSelectedObject(obj);
}

void EditorManager::ClearSelectedObject() {
    if (engine_) engine_->SetSelectedObject(nullptr);
}

void EditorManager::OnUpdate(float deltaTime) {
    if (isStepRequested_) {
        // 次のフレームで再び停止
        if (engine_) engine_->SetTimeScale(0.0f);
        isStepRequested_ = false;
    }

    if (shortcutManager_) {
        shortcutManager_->Update();
    }
}

void EditorManager::EnterPlayMode() {
    if (!engine_ || !engine_->GetSceneManager()) return;
    auto scene = engine_->GetSceneManager()->GetCurrentScene();
    if (!scene) return;

    std::string currentSceneName = engine_->GetSceneManager()->GetCurrent();
    
    // Play押下時に現在のシーンを実ファイルにも保存する
    if (!currentSceneName.empty()) {
        SceneSerializer::Save(scene, currentSceneName);
    }

    // 現在のシーン状態をバックアップ
    SceneSerializer::Save(scene, ".temp_playmode");
    playModeStartSceneName_ = currentSceneName; // 開始時のシーンを記憶

    // === ここから追加: Play開始時にシーンをクリーンな状態にリロードする ===
    ClearSelectedObject(); // 選択状態をクリア

    // GPUがすべての描画コマンドを完了するのを待機してからオブジェクトを破棄
    if (auto dxCommon = engine_->GetDirectXCommon()) {
        dxCommon->WaitForGPU();
    }
    if (auto baseScene = dynamic_cast<BaseScene*>(scene)) {
        baseScene->ClearGameObjects();
    }
    
    // 保存したばかりのバックアップから復元して、完全に初期化し直す
    SceneSerializer::Load(scene, ".temp_playmode");
    // === ここまで追加 ===

    currentMode_ = EditorModeState::Playing;
    engine_->SetPlayMode(true);
    engine_->SetTimeScale(1.0f); // 再生時は等倍
}

void EditorManager::ExitPlayMode() {
    if (!engine_ || !engine_->GetSceneManager()) return;
    
    std::string currentSceneName = engine_->GetSceneManager()->GetCurrent();

    // プレイモード中にシーンが変わっていた場合は元のシーンに戻す
    if (!playModeStartSceneName_.empty() && currentSceneName != playModeStartSceneName_) {
        engine_->GetSceneManager()->TransitionTo(playModeStartSceneName_, SceneTransition::Type::Fade, 0.0f);
        currentMode_ = EditorModeState::Edit;
        engine_->SetPlayMode(false);
        engine_->SetTimeScale(1.0f);
        return;
    }

    auto scene = engine_->GetSceneManager()->GetCurrentScene();
    if (!scene) return;

    // プレイモード中の選択状態をクリア
    ClearSelectedObject();

    // === 追加: GPUがすべての描画コマンドを完了するのを待機してからオブジェクトを破棄する ===
    // （実行中のフレームで使われているリソースが削除されることによるクラッシュを防ぐため）
    if (auto dxCommon = engine_->GetDirectXCommon()) {
        dxCommon->WaitForGPU();
    }

    if (auto baseScene = dynamic_cast<BaseScene*>(scene)) {
        baseScene->ClearGameObjects();
    }
    
    // バックアップから復元
    SceneSerializer::Load(scene, ".temp_playmode");
    currentMode_ = EditorModeState::Edit;
    engine_->SetPlayMode(false);
    engine_->SetTimeScale(1.0f);
}

void EditorManager::TogglePauseMode() {
    if (currentMode_ == EditorModeState::Playing) {
        currentMode_ = EditorModeState::Paused;
        if (engine_) engine_->SetTimeScale(0.0f); // 時を止める
    } else if (currentMode_ == EditorModeState::Paused) {
        currentMode_ = EditorModeState::Playing;
        if (engine_) engine_->SetTimeScale(1.0f); // 時を動かす
    }
}

void EditorManager::OnDrawUI() {
    if (!engine_ || !engine_->GetMainRenderTexture()) return;

    OnUpdate(engine_->GetDeltaTime()); // ここでショートカット処理を呼ぶ

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

    // --- レイアウトの初期化 (Reset Layout) ---
    static bool firstLayout = true;
    if (firstLayout) {
        firstLayout = false;
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
            resetLayout_ = true;
        }
    }

    if (resetLayout_) {
        resetLayout_ = false;
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

        ImGuiID dock_main_id = dockspaceId;
        ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.30f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("SceneView", dock_main_id);
        ImGui::DockBuilderDockWindow("Hierarchy", dock_id_left);
        ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
        ImGui::DockBuilderDockWindow("ProjectBrowser", dock_id_bottom);
        ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
        
        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // パフォーマンスパネルの表示状態
    static bool showPerformancePanel = false;

    // メニューバー
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // 現在のシーン名を取得して保存/読込
            std::string currentSceneName = "";
            if (engine_ && engine_->GetSceneManager()) {
                currentSceneName = engine_->GetSceneManager()->GetCurrent();
            }

            if (ImGui::MenuItem("Save Scene")) {
                if (engine_ && engine_->GetSceneManager() && !currentSceneName.empty()) {
                    SceneSerializer::Save(engine_->GetSceneManager()->GetCurrentScene(), currentSceneName);
                }
            }
            if (ImGui::MenuItem("Load Scene")) {
                if (engine_ && engine_->GetSceneManager() && !currentSceneName.empty()) {
                    auto* scene = engine_->GetSceneManager()->GetCurrentScene();
                    if (scene) {
                        // 既存のオブジェクトを消してからロードしたい場合はここで処理が必要
                        SceneSerializer::Load(scene, currentSceneName);
                        ClearSelectedObject(); // ロードしたら選択を解除
                    }
                }
            }

            if (ImGui::MenuItem("Exit")) {
                // 終了処理（PostQuitMessage）
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Create Empty")) actionManager_->CreatePrimitiveObject("Empty");
            if (ImGui::BeginMenu("3D Object")) {
                if (ImGui::MenuItem("Cube")) actionManager_->CreatePrimitiveObject("Cube");
                if (ImGui::MenuItem("Sphere")) actionManager_->CreatePrimitiveObject("Sphere");
                if (ImGui::MenuItem("Cylinder")) actionManager_->CreatePrimitiveObject("Cylinder");
                if (ImGui::MenuItem("Plane")) actionManager_->CreatePrimitiveObject("Plane");
                ImGui::Separator();
                if (ImGui::MenuItem("Model (MeshRenderer)")) actionManager_->CreatePrimitiveObject("Model");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("2D Object")) {
                if (ImGui::MenuItem("Sprite")) actionManager_->CreatePrimitiveObject("Sprite");
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Performance", nullptr, &showPerformancePanel);
            ImGui::Separator();
            if (ImGui::BeginMenu("Layout")) {
                if (ImGui::MenuItem("Reset Layout")) {
                    resetLayout_ = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Load Default Layout")) {
                    const char* presetPath = "../IrufemiEngine/EngineResources/default_imgui.ini";
                    const char* currentIni = ImGui::GetIO().IniFilename;
                    if (currentIni && std::filesystem::exists(presetPath)) {
                        std::error_code ec;
                        std::filesystem::copy_file(presetPath, currentIni, std::filesystem::copy_options::overwrite_existing, ec);
                        if (ec) {
                            Log::OutPutLog(std::cerr, "Failed to load preset: " + ec.message());
                            MessageBoxA(nullptr, ("Failed to load preset: " + ec.message()).c_str(), "Error", MB_OK | MB_ICONERROR);
                        } else {
                            // アプリ終了時にImGuiが現在の状態をファイルへ自動保存（上書き）してしまうのを防ぐ
                            ImGui::GetIO().IniFilename = nullptr;
                            
                            Log::OutPutLog(std::cout, "Default layout has been loaded.");
                            MessageBoxA(nullptr, "Default layout has been loaded.\nThe application will now close to apply the clean layout. Please restart the app.", "Restart Required", MB_OK | MB_ICONINFORMATION);
                            PostQuitMessage(0);
                        }
                    }
                }
                if (ImGui::MenuItem("Save Current as Default")) {
                    const char* currentIni = ImGui::GetIO().IniFilename;
                    if (currentIni) {
                        ImGui::SaveIniSettingsToDisk(currentIni);
                        const char* presetPath = "../IrufemiEngine/EngineResources/default_imgui.ini";
                        if (std::filesystem::exists(currentIni)) {
                            std::error_code ec;
                            std::filesystem::copy_file(currentIni, presetPath, std::filesystem::copy_options::overwrite_existing, ec);
                            if (ec) {
                                Log::OutPutLog(std::cerr, "Failed to save preset: " + ec.message());
                                MessageBoxA(nullptr, ("Failed to save preset: " + ec.message()).c_str(), "Error", MB_OK | MB_ICONERROR);
                            } else {
                                Log::OutPutLog(std::cout, "Default layout preset saved successfully!");
                                MessageBoxA(nullptr, "Default layout preset saved successfully!", "Success", MB_OK | MB_ICONINFORMATION);
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
        ImGui::EndMenu();
        }

        // --- 中央への Play / Pause / Step / Stop コントロール配置 ---
        float playButtonWidth = 45.0f;
        float playButtonHeight = 20.0f;
        float playButtonsTotalWidth = playButtonWidth * 4.0f + ImGui::GetStyle().ItemSpacing.x * 3.0f;
        ImGui::SameLine((ImGui::GetWindowWidth() - playButtonsTotalWidth) * 0.5f);

        // 少し下にオフセットを追加して、メニューバー内で上下の余白（パディング）を作る
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

        // Play ボタン
        if (currentMode_ == EditorModeState::Playing) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        }
        if (ImGui::Button(ICON_FA_PLAY, ImVec2(playButtonWidth, playButtonHeight))) {
            if (currentMode_ == EditorModeState::Edit) EnterPlayMode();
            else if (currentMode_ == EditorModeState::Paused) TogglePauseMode();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f); // 同行でも念のため再度オフセット

        // Pause ボタン
        if (currentMode_ == EditorModeState::Paused) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.2f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
        }
        if (ImGui::Button(ICON_FA_PAUSE, ImVec2(playButtonWidth, playButtonHeight))) {
            if (currentMode_ != EditorModeState::Edit) TogglePauseMode();
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

        // Step ボタン
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button(ICON_FA_FORWARD_STEP, ImVec2(playButtonWidth, playButtonHeight))) {
            if (currentMode_ == EditorModeState::Paused) {
                isStepRequested_ = true;
                if (engine_) engine_->SetTimeScale(1.0f); // 時を1フレームだけ動かす
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

        // Stop ボタン
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(ICON_FA_STOP, ImVec2(playButtonWidth, playButtonHeight))) {
            if (currentMode_ != EditorModeState::Edit) ExitPlayMode();
        }
        ImGui::PopStyleColor(2);

        ImGui::EndMenuBar();
    }

    // 2. 各種エディタパネルの描画
    for (auto& panel : panels_) {
        panel->Draw();
    }

    // 3. パフォーマンスパネルの描画
    if (showPerformancePanel) {
        ImGui::Begin("Performance", &showPerformancePanel, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        if (engine_) {
            ImGui::Text("Real Delta Time: %f", engine_->GetRealDeltaTime());
            ImGui::Text("Game Delta Time: %f (Scale: %.2f)", engine_->GetDeltaTime(), engine_->GetTimeScale());
        }
        ImGui::End();
    }

#ifdef USE_IMGUI
    // 描画呼び出しをDebugUI.cppに移動しました
#endif // USE_IMGUI

    ImGui::End(); // Editor DockSpace
}

#endif // EditorMode
