#include "Renderer/Pipeline/RenderGraph/TopMostPass.h"
#include "Renderer/DrawManager.h"
#include "Core/System/IrufemiEngine.h"
#include "Renderer/Pipeline/RenderGraph/RenderGraphBuilder.h"

void TopMostPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    // 最終画面（バックバッファ）に描画するため、RenderGraph管理外のリソースに直接書き込む想定
}

void TopMostPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto* cmdList = drawManager->GetDxCommon()->GetCommandList();

#ifdef EditorMode
    // EditorModeの場合はSceneViewに表示させるためMainRenderTextureに書き込む
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = engine->GetMainRenderTexture()->GetRtvHandle();
#else
    // 実行時はバックバッファに書き込む
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = drawManager->GetDxCommon()->GetRtvHandles(drawManager->GetDxCommon()->GetCurrentBackBufferIndex());
#endif
    // 深度バッファは無効化(TopMostのため)
    cmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    // ビューポートとシザーの設定を明示的に行う
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissor{};

#ifdef EditorMode
    viewport.Width = static_cast<float>(engine->GetGameResolutionWidth());
    viewport.Height = static_cast<float>(engine->GetGameResolutionHeight());
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    
    scissor.left = 0;
    scissor.right = engine->GetGameResolutionWidth();
    scissor.top = 0;
    scissor.bottom = engine->GetGameResolutionHeight();
#else
    float clientW = static_cast<float>(engine->GetClientWidth());
    float clientH = static_cast<float>(engine->GetClientHeight());
    float gameW = static_cast<float>(engine->GetGameResolutionWidth());
    float gameH = static_cast<float>(engine->GetGameResolutionHeight());

    float aspectGame = gameW / gameH;
    float aspectClient = clientW / clientH;

    if (aspectClient > aspectGame) {
        viewport.Height = clientH;
        viewport.Width = clientH * aspectGame;
        viewport.TopLeftX = (clientW - viewport.Width) * 0.5f;
        viewport.TopLeftY = 0.0f;
    } else {
        viewport.Width = clientW;
        viewport.Height = clientW / aspectGame;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = (clientH - viewport.Height) * 0.5f;
    }
    
    scissor.left = static_cast<LONG>(viewport.TopLeftX);
    scissor.right = static_cast<LONG>(viewport.TopLeftX + viewport.Width);
    scissor.top = static_cast<LONG>(viewport.TopLeftY);
    scissor.bottom = static_cast<LONG>(viewport.TopLeftY + viewport.Height);
#endif

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    // キューの描画関数を定義
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, const char* psoName) {
        if (queue.empty()) return;
        
        Irufemi::BlendMode currentBlend = Irufemi::BlendMode::kBlendModeNormal;
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
    DrawWithPSO(drawManager->GetTopMostSpriteBatchQueue(), [&](const auto& p) { drawManager->DrawTopMostSpriteBatch(p); }, "SpriteBatchForBackBuffer");

    // 最前面テキスト
    DrawWithPSO(drawManager->GetTopMostTextQueue(), [&](const auto& p) { drawManager->DrawText(p); }, "TextForBackBuffer");
}
