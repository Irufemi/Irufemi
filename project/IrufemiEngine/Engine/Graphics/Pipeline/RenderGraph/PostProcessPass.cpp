#include "PostProcessPass.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "../../PostProcess/PostProcessManager.h"
#include "../../DirectX/DirectXCommon.h"
#include "../../DirectX/DirectXUtils.h"
#include "RenderGraph.h"
void PostProcessPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    auto ppMgr = engine->GetPostProcessManager();
    const auto& activeModes = ppMgr->GetActiveModes();
    auto mainRenderTex = engine->GetMainRenderTexture();

    // 入力となるメインレンダリング結果のステート要求
    builder.RequireState(mainRenderTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    workTextureHandles_.clear();
    bloomExtractHandle_ = kInvalidHandle;
    bloomBlurHandle_ = kInvalidHandle;

    if (!activeModes.empty()) {
        bool hasOutline = false;
        bool hasBloom = false;
        for (auto mode : activeModes) {
            if (mode == PostProcessMode::Bloom) hasBloom = true;
            if (mode == PostProcessMode::DepthBasedOutline) hasOutline = true;
        }

        if (hasOutline) {
            builder.RequireState(drawManager->GetDxCommon()->GetDepthStencilResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        D3D12_RESOURCE_DESC desc = mainRenderTex->GetResource()->GetDesc();
        
        // ピンポンバッファ用の一時テクスチャ (最大2枚)
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_Work0", desc));
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_Work1", desc));


        if (hasBloom) {
            bloomExtractHandle_ = builder.CreateTransientResource("BloomExtract", desc);
            bloomBlurHandle_ = builder.CreateTransientResource("BloomBlur", desc);
        }

        // 初期ステートは全て SRV (内部で書き込み前に RTV に遷移させる)
        for (auto handle : workTextureHandles_) {
            builder.RequireTransientState(handle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (hasBloom) {
            builder.RequireTransientState(bloomExtractHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            builder.RequireTransientState(bloomBlurHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    }
}

void PostProcessPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto ppMgr = engine->GetPostProcessManager();
    auto* cmdList = drawManager->GetDxCommon()->GetCommandList();
    auto* renderGraph = drawManager->GetRenderGraph();

    PostProcessManager::PostProcessWorkspace workspace;
    
    if (!workTextureHandles_.empty()) {
        workspace.workTextures[0] = renderGraph->GetTransientRenderTexture(workTextureHandles_[0]);
        workspace.workTextures[1] = renderGraph->GetTransientRenderTexture(workTextureHandles_[1]);
    }
    if (bloomExtractHandle_ != kInvalidHandle) {
        workspace.bloomExtract = renderGraph->GetTransientRenderTexture(bloomExtractHandle_);
    }
    if (bloomBlurHandle_ != kInvalidHandle) {
        workspace.bloomBlur = renderGraph->GetTransientRenderTexture(bloomBlurHandle_);
    }

    // Outline のための逆投影行列更新
    const auto& activeModes = ppMgr->GetActiveModes();
    bool hasOutline = false;
    for (auto mode : activeModes) {
        if (mode == PostProcessMode::DepthBasedOutline) hasOutline = true;
    }
    if (hasOutline) {
        if (auto* perFrameData = drawManager->GetPerFrameData()) {
            ppMgr->GetOutlineParams().projectionInverse = Math::Inverse(perFrameData->camera.projection);
        }
    }

    // 最終出力先はバックバッファ
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = drawManager->GetDxCommon()->GetRtvHandles(drawManager->GetDxCommon()->GetCurrentBackBufferIndex());

    ppMgr->Draw(cmdList, engine->GetMainRenderTexture(), rtvHandle, workspace);

    // 深度バッファを元の DEPTH_WRITE に戻す
    if (hasOutline) {
        DirectXUtils::TransitionBarrier(
            cmdList, drawManager->GetDxCommon()->GetDepthStencilResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0
        );
    }
}
