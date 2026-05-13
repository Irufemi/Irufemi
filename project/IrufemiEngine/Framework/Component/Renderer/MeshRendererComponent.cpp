#include "MeshRendererComponent.h"
#ifdef EditorMode
#include <imgui.h>
#endif
#include "../../GameObject.h"
#include "../TransformComponent.h"
#include "Renderer/Object3D/ObjClass/ObjClass.h"

MeshRendererComponent::MeshRendererComponent() {}
MeshRendererComponent::~MeshRendererComponent() {}

void MeshRendererComponent::LoadModel(const std::string& filename) {
    modelName_ = filename;
    if (obj_) {
        obj_->Initialize(modelName_);
    }
}

void MeshRendererComponent::Initialize() {
    obj_ = std::make_unique<ObjClass>();
    obj_->Initialize(modelName_);

    // 親の GameObject から TransformComponent を探して保持しておく
    if (gameObject_) {
        transform_ = gameObject_->GetComponent<TransformComponent>();
    }
}

void MeshRendererComponent::Update() {
    // TransformComponent があれば、その座標を ObjClass に渡す（同期）
    if (transform_ && obj_) {
        obj_->SetTranslate(transform_->worldPosition_);
        obj_->SetRotate(transform_->worldRotation_);
        obj_->SetScale(transform_->worldScale_);
    }

    // ObjClass の行列計算などを実行
    if (obj_) {
        obj_->Update();
    }
}

void MeshRendererComponent::Draw() {
    // RenderGraph に向けて描画パケットを積む
    if (obj_) {
        obj_->Draw();
    }
}

#ifdef EditorMode
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include "Engine/IrufemiEngine.h"
#include "Resource/Model/ModelManager.h"

void MeshRendererComponent::OnInspectorGUI() {
    if (ImGui::TreeNodeEx("MeshRenderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        
        IrufemiEngine* engine = ObjClass::GetIrufemiEngine();
        
        if (engine && engine->GetObjModelManager()) {
            ModelManager* modelManager = engine->GetObjModelManager();
            std::vector<std::string> availableModels = modelManager->GetAvailableModels();
            
            // 現在のモデルがリストになければ追加表示用に挿入（表示上のため）
            if (std::find(availableModels.begin(), availableModels.end(), modelName_) == availableModels.end()) {
                availableModels.push_back(modelName_);
            }
            
            // モデル名と横並びでリフレッシュボタンを配置
            ImGui::Text("Model");
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60.0f);
            if (ImGui::Button("Refresh")) {
                modelManager->RefreshAvailableModels();
                availableModels = modelManager->GetAvailableModels();
                if (std::find(availableModels.begin(), availableModels.end(), modelName_) == availableModels.end()) {
                    availableModels.push_back(modelName_);
                }
            }

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            // コンボボックスでプロジェクト内の全モデルを選択
            if (ImGui::BeginCombo("##ModelCombo", modelName_.c_str())) {
                for (const auto& key : availableModels) {
                    bool isSelected = (modelName_ == key);
                    if (ImGui::Selectable(key.c_str(), isSelected)) {
                        LoadModel(key);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        } else {
            // エンジンが取得できない場合のフォールバック（通常発生しない）
            ImGui::Text("Model: %s", modelName_.c_str());
        }

        // Project Browser からの Drag & Drop (Unityスタイル)
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_PATH")) {
                std::string droppedPathStr = static_cast<const char*>(payload->Data);
                std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                std::string ext = droppedPath.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                // 3Dモデルファイルかチェック
                if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".glb") {
                    std::string newModelName = droppedPathStr;
                    std::replace(newModelName.begin(), newModelName.end(), '\\', '/');
                    // "resources/model/"からの相対パスにする (大文字小文字区別なし)
                    std::string lowerPath = newModelName;
                    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                    if (lowerPath.find("resources/model/") == 0) {
                        newModelName = newModelName.substr(16);
                    }
                    LoadModel(newModelName);
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::TextDisabled("(?) Drag & Drop model file from Project Browser");

        ImGui::TreePop();
    }
}
#endif

nlohmann::json MeshRendererComponent::Serialize() {
    nlohmann::json j;
    j["modelName"] = modelName_;
    return j;
}

void MeshRendererComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("modelName")) {
        std::string modelName = j["modelName"];
        LoadModel(modelName);
    }
}
