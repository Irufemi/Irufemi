#include "HierarchyPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "Engine/IrufemiEngine.h"
#include "Framework/SceneManager.h"
#include "Framework/BaseScene.h"
#include "Framework/GameObject.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/SpriteRendererComponent.h"

#include <functional>
#include <algorithm>

void HierarchyPanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
}

void HierarchyPanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Hierarchy");

    auto* engine = editorManager_->GetEngine();
    if (engine && engine->GetSceneManager()) {
        auto* currentScene = engine->GetSceneManager()->GetCurrentScene();
        auto* baseScene = dynamic_cast<BaseScene*>(currentScene);

        if (baseScene) {
            // 背景クリックなどで選択解除する機能
            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                editorManager_->ClearSelectedObject();
            }

            // 循環参照チェック用ラムダ
            auto IsDescendant = [](std::shared_ptr<GameObject> potentialDescendant, GameObject* ancestor) {
                if (!potentialDescendant || !ancestor) return false;
                auto current = potentialDescendant->GetParent();
                while (current) {
                    if (current.get() == ancestor) return true;
                    current = current->GetParent();
                }
                return false;
            };

            // 再帰描画用ラムダ関数
            std::function<void(std::shared_ptr<GameObject>)> DrawNode = [&](std::shared_ptr<GameObject> obj) {
                if (!obj) return;

                bool isSelected = false;
                if (auto selected = editorManager_->GetSelectedObject()) {
                    isSelected = (selected == obj);
                }

                // 描画開始時点での子の有無を保存（途中で子が追加されても不整合を起こさないため）
                bool hasChildrenAtStart = !obj->GetChildren().empty();

                // 子がいればツリーノード、いなければリーフ
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hasChildrenAtStart) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                if (isSelected) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                // 識別用にポインタアドレスを使う
                bool isOpen = ImGui::TreeNodeEx((void*)obj.get(), flags, "%s", obj->GetName().c_str());

                // クリックで選択
                if (ImGui::IsItemClicked()) {
                    editorManager_->SetSelectedObject(obj);
                }

                // --- Drag and Drop Source ---
                if (ImGui::BeginDragDropSource()) {
                    GameObject* ptr = obj.get();
                    ImGui::SetDragDropPayload("GAMEOBJECT", &ptr, sizeof(GameObject*));
                    ImGui::Text("Move %s", obj->GetName().c_str());
                    ImGui::EndDragDropSource();
                }

                // --- Drag and Drop Target ---
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
                        GameObject* payload_ptr = *(GameObject**)payload->Data;
                        
                        // 自分自身にはDropできない
                        // また、ドロップされるオブジェクトが「今の自分の親（先祖）」であってはならない（循環参照の防止）
                        if (payload_ptr != obj.get() && !IsDescendant(obj, payload_ptr)) {
                            if (auto dropObj = baseScene->FindGameObject(payload_ptr)) {
                                dropObj->SetParent(obj);
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // コンテキストメニュー (右クリック)
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::Selectable("Duplicate")) {
                        auto clone = obj->Clone();
                        baseScene->AddGameObject(clone); // BaseScene の全体リストに追加
                        if (auto parent = obj->GetParent()) {
                            clone->SetParent(parent);
                        }
                    }
                    if (ImGui::Selectable("Delete")) {
                        if (auto parent = obj->GetParent()) {
                            parent->RemoveChild(obj);
                        }
                        baseScene->RemoveGameObject(obj);
                        if (auto selected = editorManager_->GetSelectedObject()) {
                            if (selected == obj) editorManager_->ClearSelectedObject();
                        }
                    }
                    ImGui::EndPopup();
                }

                // 子ノードの描画
                if (isOpen && hasChildrenAtStart) {
                    // vector のコピーを回す（描画中に要素が削除・追加されても安全なように）
                    auto childrenCopy = obj->GetChildren();
                    for (auto& child : childrenCopy) {
                        DrawNode(child);
                    }
                    ImGui::TreePop();
                }
            };

            const auto& gameObjects = baseScene->GetGameObjects();
            auto gameObjectsCopy = gameObjects; // 描画中のリスト変更対策
            for (auto& obj : gameObjectsCopy) {
                // ルートオブジェクトのみを描画開始（子は再帰的に呼ばれる）
                if (obj && !obj->GetParent()) {
                    DrawNode(obj);
                }
            }

            // --- 余白でのD&D（ルートへ移動 ＆ アセット配置） ---
            ImGui::InvisibleButton("HierarchyDropZone", ImGui::GetContentRegionAvail());
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
                    GameObject* payload_ptr = *(GameObject**)payload->Data;
                    if (auto obj = baseScene->FindGameObject(payload_ptr)) {
                        obj->SetParent(nullptr); // 親を解除してルートに
                    }
                }
                // --- アセットのドロップを受け付ける ---
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_PATH")) {
                    std::string droppedPathStr = static_cast<const char*>(payload->Data);
                    std::filesystem::path droppedPath(reinterpret_cast<const char8_t*>(droppedPathStr.c_str()));
                    std::string ext = droppedPath.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    std::shared_ptr<GameObject> newObj = nullptr;
                    std::string stemString = reinterpret_cast<const char*>(droppedPath.stem().u8string().c_str());

                    if (ext == ".png" || ext == ".jpg" || ext == ".dds" || ext == ".bmp") {
                        newObj = std::make_shared<GameObject>("Sprite_" + stemString);
                        newObj->AddComponent<TransformComponent>();
                        auto spriteRenderer = newObj->AddComponent<SpriteRendererComponent>();
                        spriteRenderer->SetTexture(droppedPathStr); 
                        newObj->Initialize();
                    } else if (ext == ".obj" || ext == ".gltf" || ext == ".fbx" || ext == ".glb") {
                        newObj = std::make_shared<GameObject>("Model_" + stemString);
                        newObj->AddComponent<TransformComponent>();
                        auto meshRenderer = newObj->AddComponent<MeshRendererComponent>();
                        
                        // 同名ファイルに対応するため、ファイル名だけでなく相対パスを渡す
                        std::string modelName = droppedPathStr;
                        std::replace(modelName.begin(), modelName.end(), '\\', '/');
                        std::string lowerPath = modelName;
                        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
                        if (lowerPath.find("resources/model/") == 0) {
                            modelName = modelName.substr(16);
                        }
                        meshRenderer->LoadModel(modelName); 
                        newObj->Initialize();
                    }

                    if (newObj) {
                        baseScene->AddGameObject(newObj);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // --- 全体の空白での右クリック「Create」メニューを表示 ---
            if (ImGui::BeginPopupContextItem("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
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
                
                if (ImGui::BeginMenu("2D Object")) {
                    if (ImGui::Selectable("Sprite")) {
                        auto obj = std::make_shared<GameObject>("Sprite");
                        obj->AddComponent<TransformComponent>();
                        auto spriteRenderer = obj->AddComponent<SpriteRendererComponent>();
                        obj->GetComponent<TransformComponent>()->position_ = { 640.0f, 360.0f, 0.0f };
                        obj->Initialize();
                        baseScene->AddGameObject(obj);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }

            // --- Deleteキーによる削除対応 ---
            if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                if (auto selected = editorManager_->GetSelectedObject()) {
                    if (auto parent = selected->GetParent()) {
                        parent->RemoveChild(selected);
                    }
                    baseScene->RemoveGameObject(selected);
                    editorManager_->ClearSelectedObject();
                }
            }
        }
    }

    ImGui::End();
}
#endif // EditorMode
