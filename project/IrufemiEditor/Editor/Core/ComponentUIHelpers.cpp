#include "ComponentUIHelpers.h"

#ifdef EditorMode
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/CollisionManager.h"
#include "Framework/BaseScene.h"
#include "Resource/Model/ModelManager.h"
#include "Resource/Texture/TextureManager.h"
#include <algorithm>

std::shared_ptr<Component> ComponentUIHelpers::GetSharedComponent(GameObject* go, Component* comp) {
    if (!go || !comp) return nullptr;
    for (auto& c : go->GetComponents()) {
        if (c.get() == comp) return c;
    }
    return nullptr;
}

void ComponentUIHelpers::DrawCollisionLayerGUI(Component* comp, EditorActionManager* actionManager, uint32_t& layer, uint32_t& mask) {
    ImGui::Separator();
    ImGui::Text("Collision Settings");

    auto* go = comp->GetGameObject();
    auto* scene = go ? go->GetScene() : nullptr;
    auto* cm = scene ? scene->GetEngine()->GetCollisionManager() : nullptr;
    if (!cm) return;

    const auto& layerNames = cm->GetLayerNames();

    if (layerNames.empty()) return;

    int currentLayerIndex = 0;
    for (int i = 0; i < layerNames.size(); ++i) {
        if (layer == (1u << i)) {
            currentLayerIndex = i;
            break;
        }
    }

    if (ImGui::BeginCombo("Layer", layerNames[currentLayerIndex].c_str())) {
        for (int i = 0; i < layerNames.size(); ++i) {
            bool isSelected = (currentLayerIndex == i);
            if (ImGui::Selectable(layerNames[i].c_str(), isSelected)) {
                uint32_t newLayer = (1u << i);
                PushInstantUndo(actionManager, layer, newLayer, &layer);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::TreeNode("Collision Mask")) {
        if (ImGui::Button("All")) {
            PushInstantUndo(actionManager, mask, 0xFFFFFFFF, &mask);
        }
        ImGui::SameLine();
        if (ImGui::Button("None")) {
            PushInstantUndo(actionManager, mask, 0u, &mask);
        }

        for (int i = 0; i < layerNames.size(); ++i) {
            bool isMasked = (mask & (1u << i)) != 0;
            if (ImGui::Checkbox(layerNames[i].c_str(), &isMasked)) {
                uint32_t newMask = mask;
                if (isMasked) newMask |= (1u << i);
                else          newMask &= ~(1u << i);
                PushInstantUndo(actionManager, mask, newMask, &mask);
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::Button("Edit Layers...")) {
        ImGui::OpenPopup("Edit Layers");
    }

    if (ImGui::BeginPopupModal("Edit Layers", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Manage Collision Layers");
        ImGui::Separator();

        for (int i = 0; i < layerNames.size(); ++i) {
            ImGui::PushID(i);
            ImGui::Text("%2d: ", i);
            ImGui::SameLine();
            
            if (i == 0) {
                ImGui::TextDisabled("%s (Fixed)", layerNames[i].c_str());
            } else {
                char nameBuffer[64];
                strncpy_s(nameBuffer, sizeof(nameBuffer), layerNames[i].c_str(), _TRUNCATE);
                
                ImGui::SetNextItemWidth(150);
                if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer))) {
                    cm->RenameLayer(i, nameBuffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    cm->RemoveLayer(i);
                    ImGui::PopID();
                    break; 
                }
            }
            ImGui::PopID();
        }

        if (layerNames.size() < 32) {
            if (ImGui::Button("+ Add New Layer")) {
                cm->AddLayer("New Layer");
            }
        } else {
            ImGui::TextDisabled("Max layers reached (32).");
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ComponentUIHelpers::DrawFallbackPropertiesGUI(Component* component, EditorActionManager* actionManager) {
    const auto& props = component->GetProperties();
    if (props.empty()) return;
    
    ImGui::PushID(component);
    bool headerOpen = ImGui::CollapsingHeader(component->GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    
    bool pendingRemove = false;
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Remove Component")) pendingRemove = true;
        ImGui::EndPopup();
    }
    if (pendingRemove) {
        actionManager->PushAndExecute(std::make_unique<RemoveComponentCommand>(component->GetGameObject()->shared_from_this(), GetSharedComponent(component->GetGameObject(), component)));
    }

    if (headerOpen) {
        for (const auto& prop : props) {
            auto showTooltipAndReset = [&]() {
                if (!prop.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("%s", prop.tooltip.c_str());
                }
                
                bool isModified = false;
                if (!prop.defaultValue.is_null()) {
                    switch (prop.type) {
                        case ComponentPropertyType::Float: isModified = (*static_cast<float*>(prop.data) != prop.defaultValue.get<float>()); break;
                        case ComponentPropertyType::Enum:
                        case ComponentPropertyType::Int: isModified = (*static_cast<int*>(prop.data) != prop.defaultValue.get<int>()); break;
                        case ComponentPropertyType::Bool: isModified = (*static_cast<bool*>(prop.data) != prop.defaultValue.get<bool>()); break;
                        case ComponentPropertyType::String: isModified = (*static_cast<std::string*>(prop.data) != prop.defaultValue.get<std::string>()); break;
                        case ComponentPropertyType::Float2: {
                            auto* v = static_cast<Vector2*>(prop.data);
                            auto arr = prop.defaultValue;
                            if (arr.is_array() && arr.size() >= 2) isModified = (v->x != arr[0].get<float>() || v->y != arr[1].get<float>());
                            break;
                        }
                        case ComponentPropertyType::Float3: {
                            auto* v = static_cast<Vector3*>(prop.data);
                            auto arr = prop.defaultValue;
                            if (arr.is_array() && arr.size() >= 3) isModified = (v->x != arr[0].get<float>() || v->y != arr[1].get<float>() || v->z != arr[2].get<float>());
                            break;
                        }
                        case ComponentPropertyType::Float4: {
                            auto* v = static_cast<Vector4*>(prop.data);
                            auto arr = prop.defaultValue;
                            if (arr.is_array() && arr.size() >= 4) isModified = (v->x != arr[0].get<float>() || v->y != arr[1].get<float>() || v->z != arr[2].get<float>() || v->w != arr[3].get<float>());
                            break;
                        }
                        default: break;
                    }
                }

                if (isModified) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f)); // Yellow
                    if (ImGui::Button(("↺##" + prop.name).c_str())) {
                        switch (prop.type) {
                            case ComponentPropertyType::Float: *static_cast<float*>(prop.data) = prop.defaultValue.get<float>(); break;
                            case ComponentPropertyType::Enum:
                            case ComponentPropertyType::Int: *static_cast<int*>(prop.data) = prop.defaultValue.get<int>(); break;
                            case ComponentPropertyType::Bool: *static_cast<bool*>(prop.data) = prop.defaultValue.get<bool>(); break;
                            case ComponentPropertyType::String: *static_cast<std::string*>(prop.data) = prop.defaultValue.get<std::string>(); break;
                            case ComponentPropertyType::Float2: {
                                auto* v = static_cast<Vector2*>(prop.data);
                                auto arr = prop.defaultValue;
                                v->x = arr[0].get<float>(); v->y = arr[1].get<float>();
                                break;
                            }
                            case ComponentPropertyType::Float3: {
                                auto* v = static_cast<Vector3*>(prop.data);
                                auto arr = prop.defaultValue;
                                v->x = arr[0].get<float>(); v->y = arr[1].get<float>(); v->z = arr[2].get<float>();
                                break;
                            }
                            case ComponentPropertyType::Float4: {
                                auto* v = static_cast<Vector4*>(prop.data);
                                auto arr = prop.defaultValue;
                                v->x = arr[0].get<float>(); v->y = arr[1].get<float>(); v->z = arr[2].get<float>(); v->w = arr[3].get<float>();
                                break;
                            }
                            default: break;
                        }
                    }
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset to Default");
                }
            };

            bool hasValue = (prop.type != ComponentPropertyType::Header && prop.type != ComponentPropertyType::Separator);
            if (hasValue) {
                float availableWidth = ImGui::GetContentRegionAvail().x;
                ImGui::PushItemWidth(availableWidth * 0.55f);
            }

            switch (prop.type) {
                case ComponentPropertyType::Header: {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "%s", prop.name.c_str());
                    break;
                }
                case ComponentPropertyType::Separator: {
                    ImGui::Separator();
                    break;
                }
                case ComponentPropertyType::Float: {
                    float* ptr = static_cast<float*>(prop.data);
                    if (prop.minVal != prop.maxVal) {
                        ImGui::SliderFloat(prop.name.c_str(), ptr, prop.minVal, prop.maxVal);
                    } else {
                        ImGui::DragFloat(prop.name.c_str(), ptr, 0.1f);
                    }
                    CheckUndoRedoDrag(actionManager, ptr);
                    break;
                }
                case ComponentPropertyType::Enum: {
                    int* ptr = static_cast<int*>(prop.data);
                    if (!prop.enumNames.empty()) {
                        std::vector<const char*> cStrs;
                        for (const auto& s : prop.enumNames) cStrs.push_back(s.c_str());
                        int oldVal = *ptr;
                        if (ImGui::Combo(prop.name.c_str(), ptr, cStrs.data(), static_cast<int>(cStrs.size()))) {
                            PushInstantUndo(actionManager, oldVal, *ptr, ptr);
                        }
                    }
                    break;
                }
                case ComponentPropertyType::Int: {
                    int* ptr = static_cast<int*>(prop.data);
                    if (prop.minVal != prop.maxVal) {
                        ImGui::SliderInt(prop.name.c_str(), ptr, static_cast<int>(prop.minVal), static_cast<int>(prop.maxVal));
                    } else {
                        ImGui::DragInt(prop.name.c_str(), ptr, 1);
                    }
                    CheckUndoRedoDrag(actionManager, ptr);
                    break;
                }
                case ComponentPropertyType::Bool: {
                    bool* ptr = static_cast<bool*>(prop.data);
                    bool oldVal = *ptr;
                    if (ImGui::Checkbox(prop.name.c_str(), ptr)) {
                        PushInstantUndo(actionManager, oldVal, *ptr, ptr);
                    }
                    break;
                }
                case ComponentPropertyType::Float2: {
                    Vector2* ptr = reinterpret_cast<Vector2*>(prop.data);
                    ImGui::DragFloat2(prop.name.c_str(), &ptr->x, 0.1f);
                    CheckUndoRedoDrag(actionManager, ptr);
                    break;
                }
                case ComponentPropertyType::Float3: {
                    Vector3* ptr = reinterpret_cast<Vector3*>(prop.data);
                    ImGui::DragFloat3(prop.name.c_str(), &ptr->x, 0.1f);
                    CheckUndoRedoDrag(actionManager, ptr);
                    break;
                }
                case ComponentPropertyType::Float4: {
                    Vector4* ptr = reinterpret_cast<Vector4*>(prop.data);
                    if (prop.name.find("Color") != std::string::npos || prop.name.find("color") != std::string::npos) {
                        ImGui::ColorEdit4(prop.name.c_str(), &ptr->x);
                    } else {
                        ImGui::DragFloat4(prop.name.c_str(), &ptr->x, 0.1f);
                    }
                    CheckUndoRedoDrag(actionManager, ptr);
                    break;
                }
                case ComponentPropertyType::String: {
                    auto* str = static_cast<std::string*>(prop.data);
                    
                    std::string lowerName = prop.name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                    
                    bool isModel = (lowerName.find("model") != std::string::npos || lowerName.find("mesh") != std::string::npos);
                    bool isTexture = (lowerName.find("texture") != std::string::npos || lowerName.find("image") != std::string::npos);
                    
                    std::vector<std::string> comboItems;
                    IrufemiEngine* engine = nullptr;
                    if (component->GetGameObject() && component->GetGameObject()->GetScene()) {
                        engine = component->GetGameObject()->GetScene()->GetEngine();
                    }
                    
                    if (engine && isModel && engine->GetObjModelManager()) {
                        auto* mgr = engine->GetObjModelManager();
#ifndef NDEBUG
                        mgr->RefreshAvailableModels();
#endif
                        comboItems = mgr->GetAvailableModels();
                    } else if (engine && isTexture && engine->GetTextureManager()) {
                        comboItems = engine->GetTextureManager()->GetTextureNamesForDebug();
                    }

                    if (!comboItems.empty()) {
                        if (ImGui::BeginCombo(prop.name.c_str(), str->c_str())) {
                            for (const auto& item : comboItems) {
                                bool isSelected = (*str == item);
                                if (ImGui::Selectable(item.c_str(), isSelected)) {
                                    std::string oldVal = *str;
                                    *str = item;
                                    actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                                        oldVal, *str, [str](const std::string& v) { *str = v; }));
                                }
                                if (isSelected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                    } else {
                        char buffer[256];
                        strncpy_s(buffer, sizeof(buffer), str->c_str(), _TRUNCATE);
                        
                        static std::string startStr;
                        if (ImGui::InputText(prop.name.c_str(), buffer, sizeof(buffer))) {
                            *str = buffer;
                        }
                        if (ImGui::IsItemActivated()) {
                            startStr = *str;
                        }
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            std::string endStr = *str;
                            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                                startStr, endStr, [str](const std::string& v) { *str = v; }));
                        }
                    }

                    // --- Drag & Drop Support for Asset References ---
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                            const char* path = (const char*)payload->Data;
                            std::string droppedPathStr = path;
                            std::replace(droppedPathStr.begin(), droppedPathStr.end(), '\\', '/');
                            
                            std::string lowerPath = droppedPathStr;
                            std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                            
                            size_t resPos = lowerPath.find("resources/");
                            if (resPos != std::string::npos) {
                                droppedPathStr = droppedPathStr.substr(resPos);
                            }
                            
                            std::string oldVal = *str;
                            *str = droppedPathStr;
                            actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<std::string>>(
                                oldVal, droppedPathStr, [str](const std::string& v) { *str = v; }));
                        }
                        ImGui::EndDragDropTarget();
                    }
                    break;
                }
                case ComponentPropertyType::Float3Array: {
                    auto* arr = static_cast<std::vector<Vector3>*>(prop.data);
                    if (ImGui::TreeNode(prop.name.c_str())) {
                        int size = static_cast<int>(arr->size());
                        if (ImGui::InputInt("Size", &size)) {
                            if (size >= 0) {
                                std::vector<Vector3> oldArr = *arr;
                                arr->resize(size);
                                std::vector<Vector3> newArr = *arr;
                                PushInstantUndo(actionManager, oldArr, newArr, arr);
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("+")) {
                            std::vector<Vector3> oldArr = *arr;
                            arr->push_back(Vector3{0, 0, 0});
                            std::vector<Vector3> newArr = *arr;
                            PushInstantUndo(actionManager, oldArr, newArr, arr);
                        }
                        for (size_t i = 0; i < arr->size(); ++i) {
                            ImGui::PushID(static_cast<int>(i));
                            ImGui::DragFloat3("##Element", &(*arr)[i].x, 0.1f);
                            CheckUndoRedoDrag(actionManager, &(*arr)[i]);
                            
                            ImGui::SameLine();
                            if (ImGui::Button("-")) {
                                std::vector<Vector3> oldArr = *arr;
                                arr->erase(arr->begin() + i);
                                std::vector<Vector3> newArr = *arr;
                                PushInstantUndo(actionManager, oldArr, newArr, arr);
                                ImGui::PopID();
                                break; 
                            }
                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                    break;
                }
                default: break;
            }
            
            if (hasValue) {
                ImGui::PopItemWidth();
                showTooltipAndReset();
            }
        }
    }
    ImGui::PopID();
}
#endif // EditorMode
