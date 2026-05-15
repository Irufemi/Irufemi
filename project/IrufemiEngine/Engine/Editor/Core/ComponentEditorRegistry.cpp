#include "ComponentEditorRegistry.h"

#ifdef EditorMode
#include <imgui/imgui.h>
#include <filesystem>
#include <algorithm>
#include <string>

// Engine/Framework
#include "Engine/Manager/EditorManager.h"
#include "Engine/Manager/CollisionManager.h"
#include "Engine/IrufemiEngine.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"
#include "Resource/Model/ModelManager.h"
#include "Resource/Texture/TextureManager.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/GameObject.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Collider/AABBColliderComponent.h"
#include "Framework/Component/Collider/OBBColliderComponent.h"
#include "Framework/Component/Collider/SphereColliderComponent.h"
#include "Framework/Component/Collider/RaycastComponent.h"
#include "Framework/Component/Script/RotatorComponent.h"

// Core
#include "IComponentEditor.h"
#include "EditorCommands.h"
#include "EditorActionManager.h"

// --- Helper Functions ---
static void DrawCollisionLayerGUI(uint32_t& layer, uint32_t& mask) {
    ImGui::Separator();
    ImGui::Text("Collision Settings");

    auto& cm = CollisionManager::GetInstance();
    const auto& layerNames = cm.GetLayerNames();

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
                layer = (1u << i);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::TreeNode("Collision Mask")) {
        if (ImGui::Button("All")) mask = 0xFFFFFFFF;
        ImGui::SameLine();
        if (ImGui::Button("None")) mask = 0;

        for (int i = 0; i < layerNames.size(); ++i) {
            bool isMasked = (mask & (1u << i)) != 0;
            if (ImGui::Checkbox(layerNames[i].c_str(), &isMasked)) {
                if (isMasked) mask |= (1u << i);
                else          mask &= ~(1u << i);
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
                    cm.RenameLayer(i, nameBuffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    cm.RemoveLayer(i);
                    ImGui::PopID();
                    break; 
                }
            }
            ImGui::PopID();
        }

        if (layerNames.size() < 32) {
            if (ImGui::Button("+ Add New Layer")) {
                cm.AddLayer("New Layer");
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

// =======================================================================
// Custom Editors
// =======================================================================

class TransformComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<TransformComponent*>(component);
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            
            // Position
            static Vector3 startPos;
            if (ImGui::DragFloat3("Position", &comp->position_.x, 0.1f)) {
                // ドラッグ中も値は更新されるがコマンドは積まない
            }
            if (ImGui::IsItemActivated()) startPos = comp->position_;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Vector3 endPos = comp->position_;
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                    startPos, endPos, [comp](const Vector3& v) { comp->position_ = v; }));
            }
            
            // Rotation
            static Vector3 startRot;
            Vector3 rotDegrees = {
                comp->rotation_.x * (180.0f / 3.14159265f),
                comp->rotation_.y * (180.0f / 3.14159265f),
                comp->rotation_.z * (180.0f / 3.14159265f)
            };
            if (ImGui::DragFloat3("Rotation", &rotDegrees.x, 1.0f)) {
                comp->rotation_.x = rotDegrees.x * (3.14159265f / 180.0f);
                comp->rotation_.y = rotDegrees.y * (3.14159265f / 180.0f);
                comp->rotation_.z = rotDegrees.z * (3.14159265f / 180.0f);
            }
            if (ImGui::IsItemActivated()) startRot = comp->rotation_;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Vector3 endRot = comp->rotation_;
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                    startRot, endRot, [comp](const Vector3& v) { comp->rotation_ = v; }));
            }

            // Scale
            static Vector3 startScale;
            if (ImGui::DragFloat3("Scale", &comp->scale_.x, 0.1f)) {
            }
            if (ImGui::IsItemActivated()) startScale = comp->scale_;
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                Vector3 endScale = comp->scale_;
                actionManager->PushAndExecute(std::make_unique<ChangeValueCommand<Vector3>>(
                    startScale, endScale, [comp](const Vector3& v) { comp->scale_ = v; }));
            }

            ImGui::TreePop();
        }
    }
};

class MeshRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<MeshRendererComponent*>(component);
        if (ImGui::TreeNodeEx("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            IrufemiEngine* engine = BaseModel::GetIrufemiEngine();
            if (engine && engine->GetObjModelManager()) {
                ModelManager* modelManager = engine->GetObjModelManager();
                std::vector<std::string> availableModels = modelManager->GetAvailableModels();
                
                if (std::find(availableModels.begin(), availableModels.end(), comp->modelName_) == availableModels.end()) {
                    availableModels.push_back(comp->modelName_);
                }
                
                ImGui::Text("Model");
                ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60.0f);
                if (ImGui::Button("Refresh")) {
                    modelManager->RefreshAvailableModels();
                    availableModels = modelManager->GetAvailableModels();
                    if (std::find(availableModels.begin(), availableModels.end(), comp->modelName_) == availableModels.end()) {
                        availableModels.push_back(comp->modelName_);
                    }
                }

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo("##ModelCombo", comp->modelName_.c_str())) {
                    for (const auto& key : availableModels) {
                        bool isSelected = (comp->modelName_ == key);
                        if (ImGui::Selectable(key.c_str(), isSelected)) {
                            comp->LoadModel(key);
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else {
                ImGui::Text("Model: %s", comp->modelName_.c_str());
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_PATH")) {
                    std::string droppedPathStr = static_cast<const char*>(payload->Data);
                    std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                    std::string ext = droppedPath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    
                    if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".glb") {
                        std::string newModelName = droppedPathStr;
                        std::replace(newModelName.begin(), newModelName.end(), '\\', '/');
                        std::string lowerPath = newModelName;
                        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                        if (lowerPath.find("resources/model/") == 0) {
                            newModelName = newModelName.substr(16);
                        }
                        comp->LoadModel(newModelName);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::TextDisabled("(?) Drag & Drop model file from Project Browser");
            ImGui::TreePop();
        }
    }
};

class PrimitiveRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<PrimitiveRendererComponent*>(component);
        if (ImGui::TreeNodeEx("PrimitiveRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* typeNames[] = {
                "Triangle", "Plane", "Cube", "Cylinder", "Sphere", 
                "Tetra", "Circle", "Ring", "Skybox", "Cone", 
                "Torus", "IcoSphere", "Grid"
            };
            
            bool needRebuild = false;
            int typeIndex = comp->currentTypeIndex_;

            if (ImGui::Combo("Shape Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
                comp->SetShape(static_cast<PrimitiveType>(typeIndex));
                needRebuild = true;
            }

            PrimitiveType type = static_cast<PrimitiveType>(comp->currentTypeIndex_);
            switch (type) {
                case PrimitiveType::Sphere:
                case PrimitiveType::IcoSphere:
                case PrimitiveType::Circle:
                    if (ImGui::DragFloat("Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                    if (ImGui::SliderInt("Subdivisions", &comp->subdivisions_, 3, 64)) needRebuild = true;
                    break;
                case PrimitiveType::Cylinder:
                    if (ImGui::DragFloat("Top Radius", &comp->topRadius_, 0.1f, 0.0f, 100.0f)) needRebuild = true;
                    if (ImGui::DragFloat("Bottom Radius", &comp->bottomRadius_, 0.1f, 0.0f, 100.0f)) needRebuild = true;
                    if (ImGui::DragFloat("Height", &comp->height_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                    if (ImGui::SliderInt("Segments", &comp->subdivisions_, 3, 64)) needRebuild = true;
                    if (ImGui::Checkbox("Has Top", &comp->hasTop_)) needRebuild = true;
                    if (ImGui::Checkbox("Has Bottom", &comp->hasBottom_)) needRebuild = true;
                    break;
                case PrimitiveType::Cone:
                    if (ImGui::DragFloat("Radius", &comp->radius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                    if (ImGui::DragFloat("Height", &comp->height_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                    if (ImGui::SliderInt("Segments", &comp->subdivisions_, 3, 64)) needRebuild = true;
                    break;
                case PrimitiveType::Torus:
                    if (ImGui::DragFloat("Major Radius", &comp->torusMajorRadius_, 0.1f, 0.1f, 100.0f)) needRebuild = true;
                    if (ImGui::DragFloat("Minor Radius", &comp->torusMinorRadius_, 0.05f, 0.01f, 100.0f)) needRebuild = true;
                    if (ImGui::SliderInt("Major Segments", &comp->torusMajorSegments_, 3, 64)) needRebuild = true;
                    if (ImGui::SliderInt("Minor Segments", &comp->torusMinorSegments_, 3, 64)) needRebuild = true;
                    break;
            }

            if (needRebuild) {
                comp->RebuildMesh();
            }
            ImGui::TreePop();
        }
    }
};

class SpriteRendererComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<SpriteRendererComponent*>(component);
        if (ImGui::TreeNodeEx("SpriteRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool needUpdate = false;
            TextureManager* tm = Sprite::GetTextureManager();
            if (tm) {
                auto names = tm->GetTextureNamesForDebug();
                int currentIndex = 0;
                for (int i = 0; i < (int)names.size(); ++i) {
                    if (names[i] == comp->texturePath_) {
                        currentIndex = i;
                        break;
                    }
                }
                const char* currentPreview = names.empty() ? "" : names[currentIndex].c_str();
                if (ImGui::BeginCombo("Texture", currentPreview)) {
                    for (int i = 0; i < names.size(); ++i) {
                        bool isSelected = (currentIndex == i);
                        if (ImGui::Selectable(names[i].c_str(), isSelected)) {
                            comp->SetTexture(names[i]);
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else {
                char buffer[256];
                strncpy_s(buffer, sizeof(buffer), comp->texturePath_.c_str(), _TRUNCATE);
                if (ImGui::InputText("TexturePath", buffer, sizeof(buffer))) {
                    comp->SetTexture(buffer);
                }
            }
            
            if (ImGui::Checkbox("TopMost (Draw over 3D)", &comp->isTopMost_)) {
                if (comp->GetSprite()) comp->GetSprite()->SetTopMost(comp->isTopMost_);
            }
            if (ImGui::Checkbox("Flip X", &comp->isFlipX_)) needUpdate = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("Flip Y", &comp->isFlipY_)) needUpdate = true;
            
            if (needUpdate) {
                if (comp->GetSprite()) comp->GetSprite()->SetFlip(comp->isFlipX_, comp->isFlipY_);
            }

            if (ImGui::SliderFloat2("Anchor", comp->anchor_, 0.0f, 1.0f)) {
                if (comp->GetSprite()) comp->GetSprite()->SetAnchor(comp->anchor_[0], comp->anchor_[1]);
            }
            ImGui::DragFloat2("Base Size", comp->size_, 1.0f, 1.0f, 8192.0f);
            
            if (ImGui::ColorEdit4("Color", &comp->color_.x)) {
                if (comp->GetSprite()) comp->GetSprite()->SetColor(comp->color_);
            }
            ImGui::TreePop();
        }
    }
};

class AABBColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<AABBColliderComponent*>(component);
        ImGui::PushID(comp);
        if (ImGui::CollapsingHeader("AABB Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Offset", &comp->localOffset_.x, 0.1f);
            ImGui::DragFloat3("Size (Extents)", &comp->localSize_.x, 0.1f, 0.0f, 1000.0f);
            ImGui::Checkbox("Is Trigger", &comp->isTrigger_);
            DrawCollisionLayerGUI(comp->layer_, comp->mask_);
        }
        ImGui::PopID();
    }
};

class OBBColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<OBBColliderComponent*>(component);
        ImGui::PushID(comp);
        if (ImGui::CollapsingHeader("OBB Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Offset", &comp->localOffset_.x, 0.1f);
            ImGui::DragFloat3("Size (Extents)", &comp->localSize_.x, 0.1f, 0.0f, 1000.0f);
            ImGui::Checkbox("Is Trigger", &comp->isTrigger_);
            DrawCollisionLayerGUI(comp->layer_, comp->mask_);
        }
        ImGui::PopID();
    }
};

class SphereColliderComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<SphereColliderComponent*>(component);
        ImGui::PushID(comp);
        if (ImGui::CollapsingHeader("Sphere Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Offset", &comp->localOffset_.x, 0.1f);
            ImGui::DragFloat("Radius", &comp->localRadius_, 0.1f, 0.0f, 1000.0f);
            ImGui::Checkbox("Is Trigger", &comp->isTrigger_);
            DrawCollisionLayerGUI(comp->layer_, comp->mask_);
        }
        ImGui::PopID();
    }
};

class RaycastComponentEditor : public IComponentEditor {
public:
    void Draw(Component* component, EditorActionManager* actionManager) override {
        auto* comp = static_cast<RaycastComponent*>(component);
        ImGui::PushID(comp);
        if (ImGui::CollapsingHeader("Raycast", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Origin", &comp->localOffset_.x, 0.1f);
            ImGui::DragFloat3("Local Direction", &comp->localDirection_.x, 0.1f);
            ImGui::DragFloat("Max Distance", &comp->maxDistance_, 0.1f, 0.0f, 10000.0f);
            
            // RaycastはLayerの描画なし（Maskのみ指定）
            if (ImGui::TreeNode("Collision Mask")) {
                if (ImGui::Button("All")) comp->mask_ = 0xFFFFFFFF;
                ImGui::SameLine();
                if (ImGui::Button("None")) comp->mask_ = 0;

                const auto& layerNames = CollisionManager::GetInstance().GetLayerNames();
                for (int i = 0; i < layerNames.size(); ++i) {
                    bool isMasked = (comp->mask_ & (1u << i)) != 0;
                    if (ImGui::Checkbox(layerNames[i].c_str(), &isMasked)) {
                        if (isMasked) comp->mask_ |= (1u << i);
                        else          comp->mask_ &= ~(1u << i);
                    }
                }
                ImGui::TreePop();
            }
            
            // デバッグ情報
            ImGui::Separator();
            if (comp->hitInfo_.isHit) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Hit: %s", 
                    comp->hitInfo_.hitObject ? comp->hitInfo_.hitObject->GetName().c_str() : "Unknown");
                ImGui::Text("Distance: %.2f", comp->hitInfo_.distance);
                ImGui::Text("Point: (%.2f, %.2f, %.2f)", comp->hitInfo_.hitPoint.x, comp->hitInfo_.hitPoint.y, comp->hitInfo_.hitPoint.z);
            } else {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "No Hit");
            }
        }
        ImGui::PopID();
    }
};

// =======================================================================
// ComponentEditorRegistry
// =======================================================================

ComponentEditorRegistry::ComponentEditorRegistry() {}
ComponentEditorRegistry::~ComponentEditorRegistry() {}

void ComponentEditorRegistry::RegisterAllEditors() {
    RegisterEditor<TransformComponent, TransformComponentEditor>();
    RegisterEditor<MeshRendererComponent, MeshRendererComponentEditor>();
    RegisterEditor<PrimitiveRendererComponent, PrimitiveRendererComponentEditor>();
    RegisterEditor<SpriteRendererComponent, SpriteRendererComponentEditor>();
    RegisterEditor<AABBColliderComponent, AABBColliderComponentEditor>();
    RegisterEditor<OBBColliderComponent, OBBColliderComponentEditor>();
    RegisterEditor<SphereColliderComponent, SphereColliderComponentEditor>();
    RegisterEditor<RaycastComponent, RaycastComponentEditor>();
}

void ComponentEditorRegistry::DrawComponent(Component* component, EditorActionManager* actionManager) {
    if (!component) return;
    auto it = editors_.find(typeid(*component));
    if (it != editors_.end()) {
        it->second->Draw(component, actionManager);
    } else {
        // --- 簡易リフレクションによるデフォルト描画（フォールバック） ---
        const auto& props = component->GetProperties();
        if (!props.empty()) {
            ImGui::PushID(component);
            if (ImGui::CollapsingHeader(component->GetComponentName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                for (const auto& prop : props) {
                    switch (prop.type) {
                        case ComponentPropertyType::Float:
                            ImGui::DragFloat(prop.name.c_str(), static_cast<float*>(prop.data), 0.1f);
                            break;
                        case ComponentPropertyType::Int:
                            ImGui::DragInt(prop.name.c_str(), static_cast<int*>(prop.data), 1);
                            break;
                        case ComponentPropertyType::Bool:
                            ImGui::Checkbox(prop.name.c_str(), static_cast<bool*>(prop.data));
                            break;
                        case ComponentPropertyType::Float2:
                            ImGui::DragFloat2(prop.name.c_str(), reinterpret_cast<float*>(prop.data), 0.1f);
                            break;
                        case ComponentPropertyType::Float3:
                            ImGui::DragFloat3(prop.name.c_str(), reinterpret_cast<float*>(prop.data), 0.1f);
                            break;
                        case ComponentPropertyType::Float4:
                            if (prop.name.find("Color") != std::string::npos || prop.name.find("color") != std::string::npos) {
                                ImGui::ColorEdit4(prop.name.c_str(), reinterpret_cast<float*>(prop.data));
                            } else {
                                ImGui::DragFloat4(prop.name.c_str(), reinterpret_cast<float*>(prop.data), 0.1f);
                            }
                            break;
                        case ComponentPropertyType::String: {
                            auto* str = static_cast<std::string*>(prop.data);
                            char buffer[256];
                            strncpy_s(buffer, sizeof(buffer), str->c_str(), _TRUNCATE);
                            if (ImGui::InputText(prop.name.c_str(), buffer, sizeof(buffer))) {
                                *str = buffer;
                            }
                            break;
                        }
                    }
                }
            }
            ImGui::PopID();
        }
    }
}

#endif // EditorMode
