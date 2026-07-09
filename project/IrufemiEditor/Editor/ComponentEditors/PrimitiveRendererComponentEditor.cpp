#include "PrimitiveRendererComponentEditor.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include <filesystem>
#include <algorithm>
#include "../Core/ComponentUIHelpers.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/GameObject.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorCommands.h"
#include "../Core/EditorDragDrop.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"

void PrimitiveRendererComponentEditor::Draw(Component* component, EditorActionManager* actionManager) {
    auto* comp = static_cast<PrimitiveRendererComponent*>(component);
    bool headerOpen = ImGui::TreeNodeEx("PrimitiveRenderer", ImGuiTreeNodeFlags_DefaultOpen);

    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(comp->GetGameObject()->shared_from_this(), ComponentUIHelpers::GetSharedComponent(comp->GetGameObject(), comp)));
    }

    if (headerOpen) {
        if (ComponentUIHelpers::BeginPropertyTable("PrimitiveRendererTable")) {
            const char* typeNames[] = {
                "Triangle", "Plane", "Cube", "Cylinder", "Sphere", 
                "Tetra", "Circle", "Ring", "Skybox", "Cone", 
                "Torus", "IcoSphere", "Grid"
            };
            
            int typeIndex = comp->currentTypeIndex_;
            int oldTypeIndex = typeIndex;
            ImGui::TableNextRow();
            ComponentUIHelpers::DrawPropertyLabel("Shape Type");
            ImGui::TableSetColumnIndex(1);
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("##Shape Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
                ComponentUIHelpers::PushInstantUndo(actionManager, oldTypeIndex, typeIndex, std::function<void(const int&)>([comp](const int& v) { comp->SetShape(static_cast<PrimitiveType>(v)); comp->RebuildMesh(); }));
            }
            ImGui::PopItemWidth();
            ComponentUIHelpers::DrawPropertyResetButton("##ShapeReset", typeIndex != 0, [&]() {
                ComponentUIHelpers::PushInstantUndo(actionManager, oldTypeIndex, 0, std::function<void(const int&)>([comp](const int& v) { comp->SetShape(static_cast<PrimitiveType>(v)); comp->RebuildMesh(); }));
            });

            PrimitiveType type = static_cast<PrimitiveType>(comp->currentTypeIndex_);
            switch (type) {
                case PrimitiveType::Sphere:
                case PrimitiveType::IcoSphere:
                case PrimitiveType::Circle:
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->radius_, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##RadiusReset", comp->radius_ != 1.0f, [&]() {
                        float oldRadius = comp->radius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldRadius, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Subdivisions");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##Subdivisions", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##SubDivReset", comp->subdivisions_ != 16, [&]() {
                        int oldSub = comp->subdivisions_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldSub, 16, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    });
                    break;
                case PrimitiveType::Cylinder:
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Top Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Top Radius", &comp->topRadius_, 0.1f, 0.0f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->topRadius_, std::function<void(const float&)>([comp](const float& v){ comp->topRadius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##TopRadiusReset", comp->topRadius_ != 1.0f, [&]() {
                        float oldTop = comp->topRadius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldTop, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->topRadius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Bottom Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Bottom Radius", &comp->bottomRadius_, 0.1f, 0.0f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->bottomRadius_, std::function<void(const float&)>([comp](const float& v){ comp->bottomRadius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##BottomRadiusReset", comp->bottomRadius_ != 1.0f, [&]() {
                        float oldBot = comp->bottomRadius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldBot, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->bottomRadius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Height");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Height", &comp->height_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->height_, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##HeightReset", comp->height_ != 2.0f, [&]() {
                        float oldH = comp->height_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldH, 2.0f, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Segments");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##Segments", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##SegmentsReset", comp->subdivisions_ != 16, [&]() {
                        int oldSub = comp->subdivisions_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldSub, 16, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    });
                    
                    {
                        ImGui::TableNextRow();
                        ComponentUIHelpers::DrawPropertyLabel("Has Top");
                        ImGui::TableSetColumnIndex(1);
                        bool hasTop = comp->hasTop_;
                        if (ImGui::Checkbox("##Has Top", &hasTop)) {
                            ComponentUIHelpers::PushInstantUndo(actionManager, comp->hasTop_, hasTop, std::function<void(const bool&)>([comp](const bool& v){ comp->hasTop_ = v; comp->RebuildMesh(); }));
                        }
                        ComponentUIHelpers::DrawPropertyResetButton("##HasTopReset", !comp->hasTop_, [&]() {
                            bool oldTop = comp->hasTop_;
                            ComponentUIHelpers::PushInstantUndo(actionManager, oldTop, true, std::function<void(const bool&)>([comp](const bool& v){ comp->hasTop_ = v; comp->RebuildMesh(); }));
                        });

                        ImGui::TableNextRow();
                        ComponentUIHelpers::DrawPropertyLabel("Has Bottom");
                        ImGui::TableSetColumnIndex(1);
                        bool hasBottom = comp->hasBottom_;
                        if (ImGui::Checkbox("##Has Bottom", &hasBottom)) {
                            ComponentUIHelpers::PushInstantUndo(actionManager, comp->hasBottom_, hasBottom, std::function<void(const bool&)>([comp](const bool& v){ comp->hasBottom_ = v; comp->RebuildMesh(); }));
                        }
                        ComponentUIHelpers::DrawPropertyResetButton("##HasBottomReset", !comp->hasBottom_, [&]() {
                            bool oldBot = comp->hasBottom_;
                            ComponentUIHelpers::PushInstantUndo(actionManager, oldBot, true, std::function<void(const bool&)>([comp](const bool& v){ comp->hasBottom_ = v; comp->RebuildMesh(); }));
                        });
                    }
                    break;
                case PrimitiveType::Cone:
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->radius_, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##RadiusReset", comp->radius_ != 1.0f, [&]() {
                        float oldRadius = comp->radius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldRadius, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->radius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Height");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Height", &comp->height_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->height_, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##HeightReset", comp->height_ != 2.0f, [&]() {
                        float oldH = comp->height_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldH, 2.0f, std::function<void(const float&)>([comp](const float& v){ comp->height_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Segments");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##Segments", &comp->subdivisions_, 3, 64)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->subdivisions_, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##SegmentsReset", comp->subdivisions_ != 16, [&]() {
                        int oldSub = comp->subdivisions_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldSub, 16, std::function<void(const int&)>([comp](const int& v){ comp->subdivisions_ = v; comp->RebuildMesh(); }));
                    });
                    break;
                case PrimitiveType::Torus:
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Major Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Major Radius", &comp->torusMajorRadius_, 0.1f, 0.1f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMajorRadius_, std::function<void(const float&)>([comp](const float& v){ comp->torusMajorRadius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##MajorRadiusReset", comp->torusMajorRadius_ != 1.0f, [&]() {
                        float oldR = comp->torusMajorRadius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldR, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->torusMajorRadius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Minor Radius");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::DragFloat("##Minor Radius", &comp->torusMinorRadius_, 0.05f, 0.01f, 100.0f)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMinorRadius_, std::function<void(const float&)>([comp](const float& v){ comp->torusMinorRadius_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##MinorRadiusReset", comp->torusMinorRadius_ != 0.25f, [&]() {
                        float oldR = comp->torusMinorRadius_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldR, 0.25f, std::function<void(const float&)>([comp](const float& v){ comp->torusMinorRadius_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Major Segments");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##Major Segments", &comp->torusMajorSegments_, 3, 64)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMajorSegments_, std::function<void(const int&)>([comp](const int& v){ comp->torusMajorSegments_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##MajorSegReset", comp->torusMajorSegments_ != 32, [&]() {
                        int oldS = comp->torusMajorSegments_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldS, 32, std::function<void(const int&)>([comp](const int& v){ comp->torusMajorSegments_ = v; comp->RebuildMesh(); }));
                    });

                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Minor Segments");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    if (ImGui::SliderInt("##Minor Segments", &comp->torusMinorSegments_, 3, 64)) comp->RebuildMesh();
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &comp->torusMinorSegments_, std::function<void(const int&)>([comp](const int& v){ comp->torusMinorSegments_ = v; comp->RebuildMesh(); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##MinorSegReset", comp->torusMinorSegments_ != 16, [&]() {
                        int oldS = comp->torusMinorSegments_;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldS, 16, std::function<void(const int&)>([comp](const int& v){ comp->torusMinorSegments_ = v; comp->RebuildMesh(); }));
                    });
                    break;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "Material");
            ImGui::TableSetColumnIndex(1); ImGui::Separator();
            ImGui::TableSetColumnIndex(2); ImGui::Separator();
            
            if (comp->primitive_) {
                auto& mat = comp->primitive_->GetMaterial();
                
                // Texture
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Texture");
                ImGui::TableSetColumnIndex(1);
                
                TextureManager* tm = Primitive3DObject::GetTextureManager();
                if (tm) {
                    auto names = tm->GetTextureNamesForDebug();
                    int currentIndex = 0;
                    for (int i = 0; i < (int)names.size(); ++i) {
                        if (names[i] == mat.texturePath) {
                            currentIndex = i;
                            break;
                        }
                    }
                    const char* currentPreview = names.empty() ? "" : names[currentIndex].c_str();
                    ImGui::PushItemWidth(-1);
                    if (ImGui::BeginCombo("##Texture", currentPreview)) {
                        for (int i = 0; i < names.size(); ++i) {
                            bool isSelected = (currentIndex == i);
                            if (ImGui::Selectable(names[i].c_str(), isSelected)) {
                                std::string oldTex = mat.texturePath;
                                std::string newTex = names[i];
                                ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTex, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                } else {
                    char buffer[256];
                    strncpy_s(buffer, sizeof(buffer), mat.texturePath.c_str(), _TRUNCATE);
                    static std::string startTex;
                    ImGui::PushItemWidth(-1);
                    if (ImGui::InputText("##TexturePath", buffer, sizeof(buffer))) {
                        comp->SetTexture(buffer);
                    }
                    ImGui::PopItemWidth();
                    if (ImGui::IsItemActivated()) startTex = mat.texturePath;
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        std::string endTex = buffer;
                        actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                            startTex, endTex, [comp](const std::string& v){ comp->SetTexture(v); }));
                    }
                }
                
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadAssetPath)) {
                        std::string droppedPathStr = static_cast<const char*>(payload->Data);
                        std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                        std::string ext = droppedPath.extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        
                        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") {
                            std::string newTexName = droppedPathStr;
                            std::replace(newTexName.begin(), newTexName.end(), '\\', '/');
                            std::string lowerPath = newTexName;
                            std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                            if (lowerPath.find("resources/texture/") == 0) {
                                newTexName = newTexName.substr(18);
                            } else if (lowerPath.find("resources/") == 0) {
                                newTexName = newTexName.substr(10);
                            }
                            std::string oldTex = mat.texturePath;
                            ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, newTexName, std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ComponentUIHelpers::DrawPropertyResetButton("##TexReset", !mat.texturePath.empty() && mat.texturePath != "whiteTexture.png", [&]() {
                    std::string oldTex = mat.texturePath;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldTex, std::string("whiteTexture.png"), std::function<void(const std::string&)>([comp](const std::string& v){ comp->SetTexture(v); }));
                });

                // Color
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Base Color");
                ImGui::TableSetColumnIndex(1);
                ImGui::PushItemWidth(-1);
                ImGui::ColorEdit4("##Base Color", &mat.color.x);
                ImGui::PopItemWidth();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &mat.color, std::function<void(const Vector4&)>([comp](const Vector4& v){
                    comp->SetColor(v);
                }));
                ComponentUIHelpers::DrawPropertyResetButton("##ColorReset", mat.color.x != 1.0f || mat.color.y != 1.0f || mat.color.z != 1.0f || mat.color.w != 1.0f, [&]() {
                    Vector4 oldC = mat.color;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldC, Vector4{1,1,1,1}, std::function<void(const Vector4&)>([comp](const Vector4& v){ comp->SetColor(v); }));
                });

                // Lighting Mode
                const char* lightingModes[] = { "None", "Lambert", "Half-Lambert", "PBR" };
                int lightingMode = mat.lightingMode;
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Lighting Mode");
                ImGui::TableSetColumnIndex(1);
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##Lighting Mode", &lightingMode, lightingModes, IM_ARRAYSIZE(lightingModes))) {
                    ComponentUIHelpers::PushInstantUndo(actionManager, mat.lightingMode, lightingMode, std::function<void(const int&)>([comp](const int& v) { comp->SetLightingMode(v); }));
                }
                ImGui::PopItemWidth();
                ComponentUIHelpers::DrawPropertyResetButton("##LightModeReset", mat.lightingMode != 1, [&]() {
                    int oldLM = mat.lightingMode;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldLM, 1, std::function<void(const int&)>([comp](const int& v) { comp->SetLightingMode(v); }));
                });

                // Enable Lighting
                bool enableLighting = mat.enableLighting;
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Enable Lighting");
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Checkbox("##Enable Lighting", &enableLighting)) {
                    ComponentUIHelpers::PushInstantUndo(actionManager, mat.enableLighting, enableLighting, std::function<void(const bool&)>([comp](const bool& v) { comp->SetEnableLighting(v); }));
                }
                ComponentUIHelpers::DrawPropertyResetButton("##EnableLightingReset", !mat.enableLighting, [&]() {
                    bool oldEL = mat.enableLighting;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldEL, true, std::function<void(const bool&)>([comp](const bool& v) { comp->SetEnableLighting(v); }));
                });

                if (mat.lightingMode == 3) { // PBR
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Metallic");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat("##Metallic", &mat.metallic, 0.0f, 1.0f);
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &mat.metallic, std::function<void(const float&)>([comp](const float& v){ comp->SetMetallic(v); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##MetallicReset", mat.metallic != 0.0f, [&]() {
                        float oldM = mat.metallic;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldM, 0.0f, std::function<void(const float&)>([comp](const float& v){ comp->SetMetallic(v); }));
                    });
                    
                    ImGui::TableNextRow();
                    ComponentUIHelpers::DrawPropertyLabel("Roughness");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat("##Roughness", &mat.roughness, 0.0f, 1.0f);
                    ImGui::PopItemWidth();
                    ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &mat.roughness, std::function<void(const float&)>([comp](const float& v){ comp->SetRoughness(v); }));
                    ComponentUIHelpers::DrawPropertyResetButton("##RoughnessReset", mat.roughness != 1.0f, [&]() {
                        float oldR = mat.roughness;
                        ComponentUIHelpers::PushInstantUndo(actionManager, oldR, 1.0f, std::function<void(const float&)>([comp](const float& v){ comp->SetRoughness(v); }));
                    });
                }

                // Alpha Reference
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Alpha Ref");
                ImGui::TableSetColumnIndex(1);
                ImGui::PushItemWidth(-1);
                ImGui::SliderFloat("##Alpha Ref", &mat.alphaReference, 0.0f, 1.0f);
                ImGui::PopItemWidth();
                ComponentUIHelpers::CheckUndoRedoDrag(actionManager, &mat.alphaReference, std::function<void(const float&)>([comp](const float& v){ comp->SetAlphaReference(v); }));
                ComponentUIHelpers::DrawPropertyResetButton("##AlphaRefReset", mat.alphaReference != 0.01f, [&]() {
                    float oldA = mat.alphaReference;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldA, 0.01f, std::function<void(const float&)>([comp](const float& v){ comp->SetAlphaReference(v); }));
                });

                // Sampler
                const char* samplerModes[] = { "Wrap", "Clamp" };
                int samplerMode = mat.useClampSampler;
                ImGui::TableNextRow();
                ComponentUIHelpers::DrawPropertyLabel("Sampler");
                ImGui::TableSetColumnIndex(1);
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("##Sampler", &samplerMode, samplerModes, IM_ARRAYSIZE(samplerModes))) {
                    ComponentUIHelpers::PushInstantUndo(actionManager, mat.useClampSampler, samplerMode, std::function<void(const int&)>([comp](const int& v) { comp->SetUseClampSampler(v); }));
                }
                ImGui::PopItemWidth();
                ComponentUIHelpers::DrawPropertyResetButton("##SamplerReset", mat.useClampSampler != 0, [&]() {
                    int oldS = mat.useClampSampler;
                    ComponentUIHelpers::PushInstantUndo(actionManager, oldS, 0, std::function<void(const int&)>([comp](const int& v) { comp->SetUseClampSampler(v); }));
                });
            }
            ComponentUIHelpers::EndPropertyTable();
        }

        ImGui::TreePop();
    }
}
#endif // EditorMode
