#include "MainOpaquePass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "../../DirectX/ShadowMap.h"
#include "RenderGraphBuilder.h"

void MainOpaquePass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    if (auto shadowMap = drawManager->GetShadowMap()) {
        builder.RequireState(shadowMap->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

void MainOpaquePass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    // 1. Skybox
    const auto& skyboxQueue = drawManager->GetSkyboxQueue();
    if (!skyboxQueue.empty()) {
        engine->ApplySkyboxPSO();
        for (const auto& p : skyboxQueue) {
            drawManager->DrawSkybox(p);
        }
    }
    
    // Helper lambda to apply PSO efficiently
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc) {
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
                engine->ApplyPSO();
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                first = false;
            }
            drawFunc(p);
        }
    };
    
    // 2. Standard 3D (Opaque and Alpha blend)
    DrawWithPSO(drawManager->GetStandard3DQueue(), [&](const DrawManager::Standard3DPacket& p) { drawManager->DrawStandard3D(p); });
    
    // 3. Region
    DrawWithPSO(drawManager->GetRegionQueue(), [&](const DrawManager::RegionPacket& p) { drawManager->DrawRegion(p); });

    // 3.5 ModelRegion
    DrawWithPSO(drawManager->GetModelRegionQueue(), [&](const DrawManager::ModelRegionPacket& p) { drawManager->DrawModelRegion(p); });
}
