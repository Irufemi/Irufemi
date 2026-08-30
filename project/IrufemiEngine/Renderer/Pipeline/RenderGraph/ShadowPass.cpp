#include "Renderer/Pipeline/RenderGraph/ShadowPass.h"
#include "Core/System/IrufemiEngine.h"
#include "RHI/DirectX12/ShadowMap.h"
#include "Renderer/DrawManager.h"
#include "Renderer/Pipeline/RenderGraph/RenderGraphBuilder.h"

void ShadowPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    if (auto shadowMap = drawManager->GetShadowMap()) {
        builder.RequireState(shadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }
}

void ShadowPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    drawManager->BeginShadowPass();

    auto DrawShadowsWithPSO = [&](const auto& queue, const std::string& psoName, auto drawFunc) {
        if (queue.empty())
            return;

        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool first = true;

        for (const auto& p : queue) {
            if (!p.castShadows)
                continue;

            if (first || p.cullMode != currentCull) {
                engine->SetCull(p.cullMode);
                engine->ApplyPSO(psoName);
                currentCull = p.cullMode;
                first = false;
            }
            drawFunc(p);
        }
    };

    DrawShadowsWithPSO(drawManager->GetStandard3DQueue(), "Object3D",
                       [&](const auto& p) { drawManager->DrawStandard3D(p); });
    DrawShadowsWithPSO(drawManager->GetPrimitiveBatchQueue(), "Batch",
                       [&](const auto& p) { drawManager->DrawPrimitiveBatch(p); });
    DrawShadowsWithPSO(drawManager->GetModelBatchQueue(), "Batch",
                       [&](const auto& p) { drawManager->DrawModelBatch(p); });

    drawManager->EndShadowPass();
}
