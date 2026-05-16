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
#include "Framework/Component/Renderer/SpriteRendererComponent.h"
#include "Framework/Component/Renderer/TextRendererComponent.h"
#include "Framework/Component/TransformComponent.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Platform/Input/InputManager.h"
#include "Engine/Platform/Input/Mouse.h"
#include "Engine/Platform/Input/Keyboard.h"
#include <algorithm>

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
        
        ImVec2 minPos = ImGui::GetItemRectMin(); // ImGui::Image() の左上
        ImVec2 maxPos = ImGui::GetItemRectMax(); // ImGui::Image() の右下

        // --- 選択中のSpriteに対するアウトライン（強調枠）描画 ---
        if (auto selectedObj = editorManager_->GetSelectedObject()) {
            if (auto spriteComp = selectedObj->GetComponent<SpriteRendererComponent>()) {
                if (auto transform = selectedObj->GetComponent<TransformComponent>()) {
                    auto sprite = spriteComp->GetSprite();
                    if (sprite) {
                        Vector2 sizeScaled = sprite->GetSize();
                        Vector2 anchor = sprite->GetAnchor();
                        Vector3 pos = transform->worldPosition_;
                        
                        // 1280x720 空間での矩形を計算
                        float left = pos.x - sizeScaled.x * anchor.x;
                        float top = pos.y - sizeScaled.y * anchor.y;
                        float right = pos.x + sizeScaled.x * (1.0f - anchor.x);
                        float bottom = pos.y + sizeScaled.y * (1.0f - anchor.y);
                        
                        // SceneView の Image 内のローカル座標へスケール変換
                        float scaleX = size.x / 1280.0f;
                        float scaleY = size.y / 720.0f;
                        
                        // ImGuiの画面全体の座標系に変換
                        ImVec2 pMin = ImVec2(minPos.x + left * scaleX, minPos.y + top * scaleY);
                        ImVec2 pMax = ImVec2(minPos.x + right * scaleX, minPos.y + bottom * scaleY);
                        
                        // オレンジ色の枠線を描画（太さ2.0f）
                        ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(255, 165, 0, 255), 0.0f, 0, 2.0f);
                    }
                }
            } else if (auto textComp = selectedObj->GetComponent<TextRendererComponent>()) {
                if (auto transform = selectedObj->GetComponent<TransformComponent>()) {
                    Vector3 pos = transform->worldPosition_;
                    Vector2 minBounds = textComp->GetLocalBoundsMin();
                    Vector2 maxBounds = textComp->GetLocalBoundsMax();
                    
                    // Transformのスケールを適用
                    float left = pos.x + minBounds.x * transform->worldScale_.x;
                    float right = pos.x + maxBounds.x * transform->worldScale_.x;
                    float top = pos.y + minBounds.y * transform->worldScale_.y;
                    float bottom = pos.y + maxBounds.y * transform->worldScale_.y;
                    
                    float scaleX = size.x / 1280.0f;
                    float scaleY = size.y / 720.0f;
                    
                    ImVec2 pMin = ImVec2(minPos.x + left * scaleX, minPos.y + top * scaleY);
                    ImVec2 pMax = ImVec2(minPos.x + right * scaleX, minPos.y + bottom * scaleY);
                    
                    // テキストは白が多いので、オレンジではなく視認性の高い枠線（水色など）を描画
                    ImGui::GetWindowDrawList()->AddRect(pMin, pMax, IM_COL32(0, 255, 255, 255), 0.0f, 0, 2.0f);
                }
            }
        }

        
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
        
        // --- UI用の仮想マウス座標更新 & クリックによる3Dピッキング ---
        // パネルがホバーされている間は常に判定を行う
        if (ImGui::IsWindowHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos(); // 画面全体の座標

            // 画像領域内がホバーされているか判定
            if (mousePos.x >= minPos.x && mousePos.x <= maxPos.x &&
                mousePos.y >= minPos.y && mousePos.y <= maxPos.y) {
                
                // Image内のローカル座標に変換
                Vector2 localMousePos = { mousePos.x - minPos.x, mousePos.y - minPos.y };
                
                // UIなどの汎用コンポーネント用にInputManagerへ仮想座標を登録
                // （ゲームの本来の解像度 1280x720 にスケールを合わせる）
                float scaleX = 1280.0f / size.x;
                float scaleY = 720.0f / size.y;
                Vector2 scaledVirtualPos = { localMousePos.x * scaleX, localMousePos.y * scaleY };
                engine->GetInputManager()->SetVirtualMousePosition(scaledVirtualPos, true);
                
                // --- エディタカメラ操作 (Blenderライク) ---
                if (auto camera = engine->GetCameraManager()->GetActiveCamera()) {
                    auto* input = engine->GetInputManager();
                    bool isMiddleButtonDown = input->IsMouseButtonDown(Mouse::Button::Middle);
                    auto* keyboard = input->GetKeyboard();
                    bool isShiftDown = keyboard->IsKeyDown(VK_LSHIFT) || keyboard->IsKeyDown(VK_RSHIFT);
                    Vector2 mouseDelta = input->GetMouseDelta();
                    
                    // 初期化されていない場合は現在のカメラからTarget等を逆算する
                    if (!isCameraInitialized_) {
                        cameraTarget_ = {0.0f, 0.0f, 0.0f};
                        cameraDistance_ = 50.0f;
                        
                        Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(camera->GetRotate());
                        Vector3 offset = { 0.0f, 0.0f, -cameraDistance_ };
                        offset = Math::TransformNormal(offset, rotMat);
                        cameraTarget_ = Math::Subtract(camera->GetTranslate(), offset);
                        
                        isCameraInitialized_ = true;
                    }
                    
                    bool cameraChanged = false;
                    
                    if (isMiddleButtonDown) {
                        if (isShiftDown) {
                            // パン操作 (Shift + 中ボタンドラッグ)
                            const float panSpeed = 0.05f;
                            Matrix4x4 viewInverse = Math::Inverse(camera->GetViewMatrix());
                            Vector3 right = { viewInverse.m[0][0], viewInverse.m[0][1], viewInverse.m[0][2] };
                            Vector3 up = { viewInverse.m[1][0], viewInverse.m[1][1], viewInverse.m[1][2] };
                            cameraTarget_ = Math::Add(cameraTarget_, Math::Multiply(-panSpeed * mouseDelta.x, right));
                            cameraTarget_ = Math::Add(cameraTarget_, Math::Multiply(panSpeed * mouseDelta.y, up));
                            cameraChanged = true;
                        } else {
                            // オービット操作 (中ボタンドラッグ)
                            const float rotationSpeed = 0.005f;
                            Vector3 rotate = camera->GetRotate();
                            rotate.y += mouseDelta.x * rotationSpeed;
                            rotate.x += mouseDelta.y * rotationSpeed;
                            // X軸回転を制限
                            rotate.x = Math::Clamp(rotate.x, -Math::PIDiv2, Math::PIDiv2);
                            camera->SetRotate(rotate);
                            cameraChanged = true;
                        }
                    }
                    
                    // ズーム操作 (マウスホイール)
                    float wheelDelta = input->GetMouseWheelDelta();
                    if (wheelDelta != 0.0f) {
                        const float zoomSpeed = 2.0f;
                        cameraDistance_ -= wheelDelta * zoomSpeed;
                        if (cameraDistance_ < 1.0f) cameraDistance_ = 1.0f; // 最小距離制限
                        cameraChanged = true;
                    }
                    
                    // カメラの位置を更新
                    if (cameraChanged || isMiddleButtonDown) {
                        Vector3 rotate = camera->GetRotate();
                        Matrix4x4 rotMat = Math::MakeRotateXYZMatrix(rotate);
                        Vector3 offset = { 0.0f, 0.0f, -cameraDistance_ };
                        offset = Math::TransformNormal(offset, rotMat);
                        camera->SetTranslate(Math::Add(cameraTarget_, offset));
                        camera->UpdateMatrix();
                    }
                }

                // クリックされた瞬間のみピッキング判定を行う
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    bool isHit = false;
                    GameObject* closestObj = nullptr;
                    float closestDist = 1000.0f;

                    // --- 1. まず 2D (Sprite) のピッキング判定を行う ---
                    if (auto scene = engine->GetSceneManager()->GetCurrentScene()) {
                        auto gameObjects = scene->GetGameObjects();
                        // 逆順（後から追加された＝手前に描画されるもの）から判定
                        for (auto it = gameObjects.rbegin(); it != gameObjects.rend(); ++it) {
                            auto& obj = *it;
                            if (!obj || obj->IsDestroyed() || !obj->GetIsActive()) continue;
                            
                            if (auto spriteComp = obj->GetComponent<SpriteRendererComponent>()) {
                                if (auto transform = obj->GetComponent<TransformComponent>()) {
                                    auto sprite = spriteComp->GetSprite();
                                    if (sprite) {
                                        Vector2 sizeScaled = sprite->GetSize();
                                        Vector2 anchor = sprite->GetAnchor();
                                        Vector3 pos = transform->worldPosition_;
                                        
                                        float left = pos.x - sizeScaled.x * anchor.x;
                                        float top = pos.y - sizeScaled.y * anchor.y;
                                        float right = pos.x + sizeScaled.x * (1.0f - anchor.x);
                                        float bottom = pos.y + sizeScaled.y * (1.0f - anchor.y);
                                        
                                        if (scaledVirtualPos.x >= left && scaledVirtualPos.x <= right &&
                                            scaledVirtualPos.y >= top && scaledVirtualPos.y <= bottom) {
                                            closestObj = obj.get();
                                            isHit = true;
                                            break; // 2DのUIが見つかったら最優先
                                        }
                                    }
                                }
                            }
                            
                            // Textのピッキング判定
                            if (!isHit) {
                                if (auto textComp = obj->GetComponent<TextRendererComponent>()) {
                                    if (auto transform = obj->GetComponent<TransformComponent>()) {
                                        Vector3 pos = transform->worldPosition_;
                                        Vector2 minBounds = textComp->GetLocalBoundsMin();
                                        Vector2 maxBounds = textComp->GetLocalBoundsMax();
                                        
                                        float left = pos.x + minBounds.x * transform->worldScale_.x;
                                        float right = pos.x + maxBounds.x * transform->worldScale_.x;
                                        float top = pos.y + minBounds.y * transform->worldScale_.y;
                                        float bottom = pos.y + maxBounds.y * transform->worldScale_.y;
                                        
                                        if (scaledVirtualPos.x >= left && scaledVirtualPos.x <= right &&
                                            scaledVirtualPos.y >= top && scaledVirtualPos.y <= bottom) {
                                            closestObj = obj.get();
                                            isHit = true;
                                            break; // 見つかったら最優先
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // --- 2. Sprite に当たらなかった場合のみ 3D のピッキングを行う ---
                    if (!isHit) {
                        if (auto camera = engine->GetCameraManager()->GetActiveCamera()) {
                            // プロジェクション・ビュー逆行列を計算
                            Matrix4x4 viewProj = camera->GetViewProjectionMatrix3D();
                            Matrix4x4 viewProjInverse = Math::Inverse(viewProj);

                            // レイの生成
                            Ray ray = Math::ScreenPointToRay(localMousePos, size.x, size.y, viewProjInverse);

                            RaycastHit hit;
                            // まずコライダーでRaycastを実行
                            if (CollisionManager::GetInstance().Raycast(ray, hit, 1000.0f)) {
                                closestDist = hit.distance;
                                closestObj = hit.hitObject;
                                isHit = true;
                            }
                            
                            // コライダーを持たない描画コンポーネントのみのオブジェクトも判定
                            if (auto scene = engine->GetSceneManager()->GetCurrentScene()) {
                                for (auto& obj : scene->GetGameObjects()) {
                                    if (!obj || obj.get() == closestObj) continue;
                                    
                                    float dist = 0.0f;
                                    for (auto& comp : obj->GetComponents()) {
                                        if (comp->Raycast(ray, dist)) {
                                            if (dist < closestDist) {
                                                closestDist = dist;
                                                closestObj = obj.get();
                                                isHit = true;
                                            }
                                        }
                                    }
                                }
                            }
                        } // End of if (auto camera)
                    }

                    // 最終的な選択の反映
                    if (isHit && closestObj) {
                        editorManager_->SetSelectedObject(closestObj->shared_from_this());
                    } else {
                        editorManager_->ClearSelectedObject();
                    }
                } // End of if (ImGui::IsMouseClicked)
            
        } else {
            // 画像領域外なら仮想マウスを無効化
            engine->GetInputManager()->SetVirtualMousePosition({0.0f, 0.0f}, false);
        }
    } else {
        // パネルがホバーされていない場合は仮想マウスを無効化
        if (engine && engine->GetInputManager()) {
            engine->GetInputManager()->SetVirtualMousePosition({0.0f, 0.0f}, false);
        }
    }
    }

    ImGui::End();
}
#endif // EditorMode
