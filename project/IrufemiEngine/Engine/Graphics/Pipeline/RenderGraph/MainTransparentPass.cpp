#include "MainTransparentPass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"

void MainTransparentPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, bool isParticle = false, bool isLine = false) {
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
                if (isParticle) engine->ApplyParticlePSO();
                else if (isLine) engine->ApplyLineInstancedPSO();
                
                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                first = false;
            }
            drawFunc(p);
        }
    };

    // 4. Line
    DrawWithPSO(drawManager->GetLineQueue(), [&](const DrawManager::LinePacket& p) { drawManager->DrawLineInstanced(p); }, false, true);

    // 5. Particles
    DrawWithPSO(drawManager->GetParticleQueue(), [&](const DrawManager::ParticlePacket& p) { drawManager->DrawParticle(p); }, true, false);

    // 6. GPU Particles
    const auto& gpuParticleQueue = drawManager->GetGPUParticleQueue();
    if (!gpuParticleQueue.empty()) {
        BlendMode currentBlend = BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        bool first = true;
        for (const auto& p : gpuParticleQueue) {
            if (first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);
                engine->ApplyGpuParticlePSO();
                currentBlend = p.blendMode; currentDepth = p.depthWrite; currentCull = p.cullMode;
                first = false;
            }
            drawManager->DrawGPUParticle(p);
        }
    }
    // 7. Voxel Particles
    const auto& voxelParticleQueue = drawManager->GetVoxelParticleQueue();
    if (!voxelParticleQueue.empty()) {
        for (const auto& p : voxelParticleQueue) {
            drawManager->DrawVoxelParticle(p);
        }
    }
}
