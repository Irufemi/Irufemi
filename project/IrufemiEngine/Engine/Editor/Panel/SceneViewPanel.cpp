#include "SceneViewPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Engine/Manager/CollisionManager.h"
#include "../Core/EditorActionManager.h"
#include "../Core/EditorDragDrop.h"
#include "Engine/Core/Math/MathFunction.h"
#include "Framework/GameObject.h"
#include "Framework/IScene.h"
#include "Framework/Component/Renderer/MeshRendererComponent.h"
#include "Framework/Component/Renderer/PrimitiveRendererComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"

void SceneViewPanel::Initialize(EditorManager* editorManager) {
    editorManager_ = editorManager;
}

void SceneViewPanel::Draw() {
    if (!editorManager_) return;

    ImGui::Begin("Scene");

    // デバッグ線の描画ON/OFF
    bool* drawCollider = CollisionManager::GetInstance().GetIsDrawDebugLinePtr();
    if (drawCollider) {
        ImGui::Checkbox("Draw Colliders", drawCollider);
    }

    // エンジンからメインの描画結果（RenderTexture）を取得して画像として表示
    auto* engine = editorManager_->GetEngine();
    if (engine && engine->GetMainRenderTexture()) {
        auto mainTexture = engine->GetMainRenderTexture();
        
        // パネルの大きさを取得して画像をフィットさせる
        ImVec2 size = ImGui::GetContentRegionAvail();
        // 画像アスペクト比を維持したい場合は別途計算が必要だが、今回はパネルいっぱいに描画
        ImGui::Image((ImTextureID)mainTexture->GetSrvHandleGPU().ptr, size);
        
        // --- アセットのドラッグ＆ドロップを受け付ける ---
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadAssetPath)) {
                std::string droppedPathStr = static_cast<const char*>(payload->Data);
                
                // EditorActionManager に生成を依頼（Undo履歴にも自動登録される）
                if (auto am = editorManager_->GetActionManager()) {
                    am->CreateObjectFromAsset(droppedPathStr);
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        // --- クリックによるピッキング (Raycast) ---
        // パネルがホバーされていて、左クリックされた瞬間のみ判定
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mousePos = ImGui::GetMousePos(); // 画面全体の座標
            ImVec2 minPos = ImGui::GetItemRectMin(); // ImGui::Image() の左上
            ImVec2 maxPos = ImGui::GetItemRectMax(); // ImGui::Image() の右下

            // 画像領域内がクリックされたか判定
            if (mousePos.x >= minPos.x && mousePos.x <= maxPos.x &&
                mousePos.y >= minPos.y && mousePos.y <= maxPos.y) {
                
                // Image内のローカル座標に変換
                Vector2 localMousePos = { mousePos.x - minPos.x, mousePos.y - minPos.y };
                
                if (auto camera = engine->GetCameraManager()->GetActiveCamera()) {
                    // プロジェクション・ビュー逆行列を計算
                    Matrix4x4 viewProj = camera->GetViewProjectionMatrix3D();
                    Matrix4x4 viewProjInverse = Math::Inverse(viewProj);

                    // レイの生成
                    Ray ray = Math::ScreenPointToRay(localMousePos, size.x, size.y, viewProjInverse);

                    RaycastHit hit;
                    // まずコライダーでRaycastを実行
                    bool isHit = CollisionManager::GetInstance().Raycast(ray, hit, 1000.0f);
                    
                    float closestDist = isHit ? hit.distance : 1000.0f;
                    GameObject* closestObj = isHit ? hit.hitObject : nullptr;

                    // コライダーを持たない描画コンポーネントのみのオブジェクトも判定
                    if (auto scene = engine->GetSceneManager()->GetCurrentScene()) {
                        for (auto& obj : scene->GetGameObjects()) {
                            if (!obj || obj.get() == closestObj) continue;
                            
                            Sphere bounds;
                            bool hasBounds = false;
                            
                            if (auto meshRenderer = obj->GetComponent<MeshRendererComponent>()) {
                                bounds = meshRenderer->GetWorldSphere();
                                hasBounds = true;
                            } else if (auto primitiveRenderer = obj->GetComponent<PrimitiveRendererComponent>()) {
                                bounds = primitiveRenderer->GetWorldSphere();
                                hasBounds = true;
                            }
                            
                            if (hasBounds) {
                                float dist = 0.0f;
                                if (Collision::IsCollision(ray, bounds, dist)) {
                                    if (dist < closestDist) {
                                        closestDist = dist;
                                        closestObj = obj.get();
                                        isHit = true;
                                    }
                                }
                            }
                        }
                    }

                    if (isHit && closestObj) {
                        editorManager_->SetSelectedObject(closestObj->shared_from_this());
                    } else {
                        editorManager_->ClearSelectedObject();
                    }
                }
            }
        }
    }

    ImGui::End();
}
#endif // EditorMode
