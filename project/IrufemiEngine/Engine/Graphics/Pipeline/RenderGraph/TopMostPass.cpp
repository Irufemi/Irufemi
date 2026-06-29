#include "TopMostPass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "RenderGraphBuilder.h"

void TopMostPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    // 最終画面（バックバッファ）に描画するため、RenderGraph管理外のリソースに直接書き込む想定
}

void TopMostPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto* cmdList = drawManager->GetDxCommon()->GetCommandList();

    // バックバッファをレンダーターゲットに設定
    drawManager->SetRenderTargetToBackBuffer(false);

    // キューの描画関数を定義
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, const char* psoName) {
        if (queue.empty()) return;
        
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool psoApplied = false;
        bool first = true;
        
        for (const auto& p : queue) {
            bool stateChanged = first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull;
            
            if (stateChanged || !psoApplied) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                engine->ApplyPSO(psoName);
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                psoApplied = true;
                first = false;
            }
            drawFunc(p);
        }
    };

    // 最前面スプライト
    DrawWithPSO(drawManager->GetTopMostSpriteQueue(), [&](const auto& p) { drawManager->DrawSprite(p); }, "SpriteForBackBuffer");

    // 最前面スプライトバッチ
    DrawWithPSO(drawManager->GetTopMostSpriteBatchQueue(), [&](const auto& p) { drawManager->DrawTopMostSpriteBatch(p); }, "SpriteBatch");

    // 最前面テキスト
    DrawWithPSO(drawManager->GetTopMostTextQueue(), [&](const auto& p) { drawManager->DrawText(p); }, "Text");
}
