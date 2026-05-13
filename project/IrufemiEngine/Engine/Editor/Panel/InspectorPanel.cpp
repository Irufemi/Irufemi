#include "InspectorPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/SceneManager.h"
#include "Framework/BaseScene.h"
#include "Framework/GameObject.h"
#include "../Core/EditorTheme.h"
#include "../Core/ComponentEditorRegistry.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include <cstring>

void InspectorPanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
}

void InspectorPanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Inspector");

    if (auto selected = editorManager_->GetSelectedObject()) {
        char nameBuffer[256];
        strncpy_s(nameBuffer, selected->GetName().c_str(), sizeof(nameBuffer) - 1);
        
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 150); // Deleteボタンのスペースを確保
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            selected->SetName(nameBuffer);
        }
        
        // --- オブジェクト削除ボタン（赤色で右端に配置） ---
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        EditorTheme::PushDangerButtonStyle();
        if (ImGui::Button("Delete", ImVec2(70, 0))) {
            auto* engine = editorManager_->GetEngine();
            if (engine && engine->GetSceneManager()) {
                if (auto* baseScene = dynamic_cast<BaseScene*>(engine->GetSceneManager()->GetCurrentScene())) {
                    baseScene->RemoveGameObject(selected);
                    editorManager_->ClearSelectedObject(); // 選択解除
                }
            }
        }
        EditorTheme::PopButtonStyle();
        // ------------------------------------------------

        ImGui::Separator();

        // コンポーネントのUIを描画
        if (auto sel = editorManager_->GetSelectedObject()) {
            if (auto registry = editorManager_->GetComponentEditorRegistry()) {
                for (const auto& comp : sel->GetComponents()) {
                    registry->DrawComponent(comp.get());
                }
            }

            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("Add Component", ImVec2(-1, 30))) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                bool hasTransform = sel->GetComponent<TransformComponent>() != nullptr;
                bool hasMeshRenderer = sel->GetComponent<MeshRendererComponent>() != nullptr;
                bool hasPrimitiveRenderer = sel->GetComponent<PrimitiveRendererComponent>() != nullptr;
                bool hasSpriteRenderer = sel->GetComponent<SpriteRendererComponent>() != nullptr;
                bool hasAnyRenderer = hasMeshRenderer || hasPrimitiveRenderer || hasSpriteRenderer;

                if (!hasTransform) {
                    if (ImGui::Selectable("TransformComponent")) sel->AddComponent<TransformComponent>();
                } else {
                    ImGui::TextDisabled("TransformComponent (Already added)");
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Renderer")) {
                    if (!hasAnyRenderer) {
                        if (ImGui::Selectable("MeshRendererComponent")) sel->AddComponent<MeshRendererComponent>();
                        if (ImGui::Selectable("PrimitiveRendererComponent")) sel->AddComponent<PrimitiveRendererComponent>();
                        if (ImGui::Selectable("SpriteRendererComponent")) sel->AddComponent<SpriteRendererComponent>();
                    } else {
                        if (hasMeshRenderer) ImGui::TextDisabled("MeshRendererComponent (Already added)");
                        else if (hasPrimitiveRenderer) ImGui::TextDisabled("PrimitiveRendererComponent (Already added)");
                        else if (hasSpriteRenderer) ImGui::TextDisabled("SpriteRendererComponent (Already added)");
                        ImGui::Separator();
                        ImGui::TextDisabled("Only one renderer is allowed.");
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Collider")) {
                    if (!sel->GetComponent<AABBColliderComponent>()) {
                        if (ImGui::Selectable("AABBColliderComponent")) sel->AddComponent<AABBColliderComponent>();
                    } else { ImGui::TextDisabled("AABBColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<SphereColliderComponent>()) {
                        if (ImGui::Selectable("SphereColliderComponent")) sel->AddComponent<SphereColliderComponent>();
                    } else { ImGui::TextDisabled("SphereColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<OBBColliderComponent>()) {
                        if (ImGui::Selectable("OBBColliderComponent")) sel->AddComponent<OBBColliderComponent>();
                    } else { ImGui::TextDisabled("OBBColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<RaycastComponent>()) {
                        if (ImGui::Selectable("RaycastComponent")) sel->AddComponent<RaycastComponent>();
                    } else { ImGui::TextDisabled("RaycastComponent (Already added)"); }
                    ImGui::EndMenu();
                }
                
                ImGui::Separator();
                
                if (ImGui::BeginMenu("Remove Component")) {
                    bool hasRemovable = false;
                    for (auto& comp : sel->GetComponents()) {
                        if (!comp) continue;
                        std::string compName = comp->GetComponentName();
                        if (compName == "TransformComponent") continue; // Transformは削除不可
                        
                        hasRemovable = true;
                        if (ImGui::Selectable(compName.c_str())) {
                            sel->RemoveComponent(comp.get());
                            ImGui::EndMenu();
                            ImGui::EndPopup();
                            break;
                        }
                    }
                    if (!hasRemovable) {
                        ImGui::TextDisabled("No removable components.");
                    } else {
                        ImGui::EndMenu();
                    }
                } else {
                    // Remove Component menu didn't begin, so we do nothing here
                }
                
                ImGui::EndPopup();
            }
        }
    } else {
        ImGui::Text("No object selected.");
    }

    ImGui::End();
}
#endif // EditorMode
