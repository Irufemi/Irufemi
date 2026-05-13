#include "SceneViewPanel.h"

#ifdef EditorMode
#include "imgui/imgui.h"
#include "Engine/Manager/EditorManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Graphics/DirectX/RenderTexture.h"
#include "Engine/Manager/CollisionManager.h"

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
    }

    ImGui::End();
}
#endif // EditorMode
