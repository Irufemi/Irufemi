#include "Renderer/Pipeline/RenderGraph/MainTransparentPass.h"
#include "Core/System/IrufemiEngine.h"
#include "RHI/DirectX12/DirectXUtils.h"
#include "RHI/DirectX12/RootSignatureConfig.h"
#include "RHI/DirectX12/ShadowMap.h"
#include "Renderer/DrawManager.h"
#include "Renderer/Pipeline/RenderGraph/RenderGraphBuilder.h"
#include <algorithm>

void MainTransparentPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    if (auto shadowMap = drawManager->GetShadowMap()) {
        builder.RequireState(shadowMap->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // G-Bufferをレンダーターゲットとして要求
    if (auto tex = engine->GetMainRenderTexture())
        builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (auto tex = engine->GetEffectMaskTexture())
        builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (auto tex = engine->GetNormalTexture())
        builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (auto tex = engine->GetMaterialTexture())
        builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    if (auto tex = engine->GetVelocityTexture())
        builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 深度バッファを書き込み可能として要求
    if (auto dx = drawManager->GetDxCommon()) {
        builder.RequireState(dx->GetDepthStencilResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // GPUParticleのリソースを読み取り専用として要求
    for (const auto& p : drawManager->GetGPUParticleQueue()) {
        if (p.particleResource) {
            builder.RequireState(p.particleResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                                                         D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }
    }
}

void MainTransparentPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto cmdList = engine->GetCommandList();
    auto dxCommon = engine->GetDirectXCommon();
    auto depthResource = dxCommon->GetDepthStencilResource();

    // 1. バリア: DEPTH_WRITE -> DEPTH_READ | PIXEL_SHADER_RESOURCE
    if (depthResource) {
        DirectXUtils::TransitionBarrier(cmdList, depthResource, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    // 2. ReadOnly DSV をセット (初期状態は2つのRTV)
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2] = {engine->GetMainRenderTexture()->GetRtvHandle(),
                                                 engine->GetEffectMaskTexture()->GetRtvHandle()};
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleSingle = rtvHandles[0];
    D3D12_CPU_DESCRIPTOR_HANDLE readOnlyDsvHandle = dxCommon->GetReadOnlyDSVCPUDescriptorHandle();

    // Transparent 3D (Object3D PSO) は MRT (2 RTV) を使用する
    cmdList->OMSetRenderTargets(2, rtvHandles, false, &readOnlyDsvHandle);
    cmdList->SetGraphicsRootDescriptorTable(static_cast<UINT>(RootSlot::DepthMap), dxCommon->GetDepthSRVGPUHandle());

    auto DrawWithPSO = [&](const auto& queue, auto drawFunc, bool isParticle = false, bool isLine = false,
                           bool isDebugPrimitive = false) {
        if (queue.empty())
            return;

        Irufemi::BlendMode currentBlend = Irufemi::BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        ID3D12PipelineState* currentCustomPSO = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS currentCustomCBV = 0;
        bool psoApplied = false;
        bool first = true;

        for (const auto& p : queue) {
            bool stateChanged =
                first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull;
            bool psoChanged = (p.customPSO != currentCustomPSO);

            if (stateChanged || psoChanged || !psoApplied) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);

                if (p.customPSO) {
                    drawManager->BindPSO(p.customPSO);
                } else {
                    if (isParticle)
                        engine->ApplyPSO("Particle");
                    else if (isLine)
                        engine->ApplyPSO("LineBatch");
                    else if (isDebugPrimitive)
                        engine->ApplyPSO("DebugPrimitive");
                }

                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                currentCustomPSO = p.customPSO;
                currentCustomCBV = 0; // Force re-bind
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

    // 3. Transparent 3D (エフェクト・半透明) - MRT(2)が必要
    auto transparentQueue = drawManager->GetTransparent3DQueue(); // コピーしてソート
    std::sort(transparentQueue.begin(), transparentQueue.end(),
              [](const RenderPackets::Standard3DPacket& a, const RenderPackets::Standard3DPacket& b) {
                  return a.distanceToCamera > b.distanceToCamera; // 遠いものから描画 (Back-to-Front)
              });
    DrawWithPSO(transparentQueue, [&](const auto& p) { drawManager->DrawStandard3D(p); }, false, false);

    // 全て MRT(2) に対応済みのため、ここでは切り替えずにそのまま描画
    DrawWithPSO(
        drawManager->GetLineQueue(), [&](const auto& p) { drawManager->DrawLineInstanced(p); }, false, true, false);

    // 4.5 DebugPrimitive
    DrawWithPSO(
        drawManager->GetDebugPrimitiveQueue(), [&](const auto& p) { drawManager->DrawDebugPrimitive(p); }, false, false,
        true);

    // 6. GPU Particles
    const auto& gpuParticleQueue = drawManager->GetGPUParticleQueue();
    if (!gpuParticleQueue.empty()) {
        Irufemi::BlendMode currentBlend = Irufemi::BlendMode::kBlendModeNormal;
        PSOManager::DepthWrite currentDepth = PSOManager::DepthWrite::Enable;
        PSOManager::CullMode currentCull = PSOManager::CullMode::Back;
        ID3D12PipelineState* currentCustomPSO = nullptr;
        bool psoApplied = false;
        bool first = true;
        for (const auto& p : gpuParticleQueue) {
            bool stateChanged =
                first || p.blendMode != currentBlend || p.depthWrite != currentDepth || p.cullMode != currentCull;
            bool psoChanged = (p.customPSO != currentCustomPSO);

            if (stateChanged || psoChanged || !psoApplied) {
                engine->SetBlend(p.blendMode);
                engine->SetDepthWrite(p.depthWrite);
                engine->SetCull(p.cullMode);

                if (p.customPSO) {
                    drawManager->BindPSO(p.customPSO);
                } else {
                    engine->ApplyPSO("GpuParticle");
                }

                currentBlend = p.blendMode;
                currentDepth = p.depthWrite;
                currentCull = p.cullMode;
                currentCustomPSO = p.customPSO;
                psoApplied = true;
                first = false;
            }
            drawManager->DrawGPUParticle(p);
        }
    }
    // 7. Irufemi::Voxel Particles
    const auto& voxelParticleQueue = drawManager->GetVoxelParticleQueue();
    if (!voxelParticleQueue.empty()) {
        for (const auto& p : voxelParticleQueue) {
            drawManager->DrawVoxelParticle(p);
        }
    }

    // 4. バリアを元に戻す: DEPTH_READ | PIXEL_SHADER_RESOURCE -> DEPTH_WRITE
    if (depthResource) {
        DirectXUtils::TransitionBarrier(cmdList, depthResource,
                                        D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // 5. Writable DSV に戻す (以降のパスで必要になる場合のため)
    // 次のパスが使うために、ここでは2枚に戻しておく(DrawManager側で必要に応じて上書きされる)
    D3D12_CPU_DESCRIPTOR_HANDLE writableDsvHandle = dxCommon->GetDSVCPUDescriptorHandle(0);
    cmdList->OMSetRenderTargets(2, rtvHandles, false, &writableDsvHandle);
}
