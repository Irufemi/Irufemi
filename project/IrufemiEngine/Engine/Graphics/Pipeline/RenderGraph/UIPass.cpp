#include "UIPass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "RenderGraphBuilder.h"

void UIPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
#ifdef EditorMode
    // エディタモードでは、ゲーム内UI(Sprite等)も mainRenderTexture に描き込む必要があるため RENDER_TARGET を要求する
    builder.RequireState(engine->GetMainRenderTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
#endif
}

void UIPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, bool isSprite = false) {
        if (queue.empty()) return;
        
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool first = true;
        
        for (const auto& p : queue) {
            if (first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                if (isSprite) engine->ApplyPSO("Sprite");
                else engine->ApplyPSO("Object3D"); // For UI 3D
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                first = false;
            }
            drawFunc(p);
        }
    };

    // 8. Sprites
    DrawWithPSO(drawManager->GetSpriteQueue(), [&](const auto& p) { drawManager->DrawSprite(p); }, true);

    // 8.5 UI 3D Objects (Always drawn on top of Sprites)
    DrawWithPSO(drawManager->GetUI3DQueue(), [&](const auto& p) { drawManager->DrawStandard3D(p); }, false);

    // 9. Post Custom Draws
    const auto& postRenderQueue = drawManager->GetPostRenderQueue();
    for (auto& func : postRenderQueue) {
        func();
    }
}
