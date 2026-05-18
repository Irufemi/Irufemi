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
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/Component/Script/RotatorComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/Component/Renderer/ParticleEmitterComponent.h"
#include "Framework/Component/AudioSourceComponent.h"
#include "Framework/Component/Camera/CameraComponent.h"
#include "Framework/Component/UI/ButtonComponent.h"
#include "Framework/Component/UI/CanvasComponent.h"
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
        static std::string startName;
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
            selected->SetName(nameBuffer);
        }
        if (ImGui::IsItemActivated()) startName = selected->GetName();
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            std::string endName = selected->GetName();
            editorManager_->GetActionManager()->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                startName, endName, [selected](const std::string& s) { selected->SetName(s); }));
        }
        
        // --- オブジェクト削除ボタン（赤色で右端に配置） ---
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        EditorTheme::PushDangerButtonStyle();
        if (ImGui::Button("Delete", ImVec2(70, 0))) {
            if (auto actionManager = editorManager_->GetActionManager()) {
                actionManager->DeleteObject(selected);
            }
        }
        EditorTheme::PopButtonStyle();
        // ------------------------------------------------

        ImGui::Separator();

        // コンポーネントのUIを描画
        if (auto sel = editorManager_->GetSelectedObject()) {
            auto* actionManager = editorManager_->GetActionManager();
            if (auto registry = editorManager_->GetComponentEditorRegistry()) {
                for (const auto& comp : sel->GetComponents()) {
                    registry->DrawComponent(comp.get(), actionManager);
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
                bool hasTextRenderer = sel->GetComponent<TextRendererComponent>() != nullptr;
                bool hasParticleEmitter = sel->GetComponent<ParticleEmitterComponent>() != nullptr;
                bool hasAnyRenderer = hasMeshRenderer || hasPrimitiveRenderer || hasSpriteRenderer || hasTextRenderer || hasParticleEmitter;

                if (!hasTransform) {
                    if (ImGui::Selectable("TransformComponent")) {
                        auto comp = std::make_shared<TransformComponent>();
                        actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                    }
                } else {
                    ImGui::TextDisabled("TransformComponent (Already added)");
                }

                ImGui::Separator();

                if (ImGui::BeginMenu("Renderer")) {
                    if (!hasAnyRenderer) {
                        if (ImGui::Selectable("MeshRendererComponent")) {
                            auto comp = std::make_shared<MeshRendererComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                        if (ImGui::Selectable("PrimitiveRendererComponent")) {
                            auto comp = std::make_shared<PrimitiveRendererComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                        if (ImGui::Selectable("SpriteRendererComponent")) {
                            auto comp = std::make_shared<SpriteRendererComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                        if (ImGui::Selectable("TextRendererComponent")) {
                            auto comp = std::make_shared<TextRendererComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                        if (ImGui::Selectable("ParticleEmitterComponent")) {
                            auto comp = std::make_shared<ParticleEmitterComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else {
                        if (hasMeshRenderer) ImGui::TextDisabled("MeshRendererComponent (Already added)");
                        else if (hasPrimitiveRenderer) ImGui::TextDisabled("PrimitiveRendererComponent (Already added)");
                        else if (hasSpriteRenderer) ImGui::TextDisabled("SpriteRendererComponent (Already added)");
                        else if (hasTextRenderer) ImGui::TextDisabled("TextRendererComponent (Already added)");
                        else if (hasParticleEmitter) ImGui::TextDisabled("ParticleEmitterComponent (Already added)");
                        ImGui::Separator();
                        ImGui::TextDisabled("Only one renderer is allowed.");
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Collider")) {
                    if (!sel->GetComponent<AABBColliderComponent>()) {
                        if (ImGui::Selectable("AABBColliderComponent")) {
                            auto comp = std::make_shared<AABBColliderComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("AABBColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<SphereColliderComponent>()) {
                        if (ImGui::Selectable("SphereColliderComponent")) {
                            auto comp = std::make_shared<SphereColliderComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("SphereColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<OBBColliderComponent>()) {
                        if (ImGui::Selectable("OBBColliderComponent")) {
                            auto comp = std::make_shared<OBBColliderComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("OBBColliderComponent (Already added)"); }
                    
                    if (!sel->GetComponent<RaycastComponent>()) {
                        if (ImGui::Selectable("RaycastComponent")) {
                            auto comp = std::make_shared<RaycastComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("RaycastComponent (Already added)"); }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Scripts")) {
                    if (!sel->GetComponent<RotatorComponent>()) {
                        if (ImGui::Selectable("RotatorComponent")) {
                            auto comp = std::make_shared<RotatorComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("RotatorComponent (Already added)"); }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Audio")) {
                    if (!sel->GetComponent<AudioSourceComponent>()) {
                        if (ImGui::Selectable("AudioSourceComponent")) {
                            auto comp = std::make_shared<AudioSourceComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("AudioSourceComponent (Already added)"); }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Camera")) {
                    if (!sel->GetComponent<CameraComponent>()) {
                        if (ImGui::Selectable("CameraComponent")) {
                            auto comp = std::make_shared<CameraComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("CameraComponent (Already added)"); }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("UI")) {
                    if (!sel->GetComponent<ButtonComponent>()) {
                        if (ImGui::Selectable("ButtonComponent")) {
                            auto comp = std::make_shared<ButtonComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("ButtonComponent (Already added)"); }
                    
                    if (!sel->GetComponent<CanvasComponent>()) {
                        if (ImGui::Selectable("CanvasComponent")) {
                            auto comp = std::make_shared<CanvasComponent>();
                            actionManager->PushAndExecute(std::make_unique<AddComponentCommand>(sel, comp));
                        }
                    } else { ImGui::TextDisabled("CanvasComponent (Already added)"); }
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
                            actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(sel, comp));
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
