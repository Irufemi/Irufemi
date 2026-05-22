#include "UIPass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "RenderGraphBuilder.h"

void UIPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    // UIPass 自体は特別なバリアを要求しないが、念のため
}

void UIPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, bool isSprite = false) {
        if (queue.empty()) return;
        
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        ID3D12PipelineState* currentCustomPSO = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS currentCustomCBV = 0;
        bool psoApplied = false;
        bool first = true;
        
        for (const auto& p : queue) {
            bool stateChanged = first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull;
            bool psoChanged = (p.customPSO != currentCustomPSO);
            
            if (stateChanged || psoChanged || !psoApplied) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                
                if (p.customPSO) {
                    drawManager->BindPSO(p.customPSO);
                } else {
                    if (isSprite) engine->ApplyPSO("Sprite");
                    else engine->ApplyPSO("Object3D"); // For UI 3D
                }
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                currentCustomPSO = p.customPSO;
                currentCustomCBV = 0;
                psoApplied = true;
                first = false;
            }

            if (p.customCBVAddress != 0 && p.customCBVAddress != currentCustomCBV) {
                engine->BindLightningParams(p.customCBVAddress);
                currentCustomCBV = p.customCBVAddress;
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
