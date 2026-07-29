#include "PostUIPass.h"
#include "Engine/Core/Math/Math.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "../../PostProcess/PostProcessManager.h"
#include "../../DirectX/DirectXCommon.h"
#include "../../DirectX/DirectXUtils.h"
#include "RenderGraph.h"

void PostUIPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    auto ppMgr = engine->GetPostProcessManager();
    const auto& activeModes = ppMgr->GetActiveModes(PostProcessManager::Layer::PostUI);
    auto mainRenderTex = engine->GetMainRenderTexture();

    // 自身に書き戻す、またはバックバッファに書き込むための事前ステート
    builder.RequireState(mainRenderTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    // マスクバッファをシェーダーリソースとして要求する
    builder.RequireState(engine->GetEffectMaskTexture()->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    workTextureHandles_.clear();

    D3D12_RESOURCE_DESC desc = mainRenderTex->GetResource()->GetDesc();

    // 入力ソース退避用テクスチャ (エフェクトの有無に関わらず常に必要)
    postUiSrcHandle_ = builder.CreateTransientResource("PP_PostUISrc", desc);
    builder.RequireTransientState(postUiSrcHandle_, D3D12_RESOURCE_STATE_COPY_DEST);

    if (!activeModes.empty()) {
        // ピンポンバッファ用の一時テクスチャ (最大2枚)
        // ポストプロセスの中間計算はリニア空間で行うため、UNORM を指定する
        D3D12_RESOURCE_DESC workDesc = desc;
        workDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_PostUI_Work0", workDesc));
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_PostUI_Work1", workDesc));

        // 初期ステートは全て SRV (内部で書き込み前に RTV に遷移させる)
        for (auto handle : workTextureHandles_) {
            builder.RequireTransientState(handle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    }
}

void PostUIPass::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto ppMgr = engine->GetPostProcessManager();
    auto* cmdList = drawManager->GetDxCommon()->GetCommandList();
    auto* renderGraph = drawManager->GetRenderGraph();

    PostProcessManager::PostProcessWorkspace workspace;
    
    if (!workTextureHandles_.empty()) {
        workspace.workTextures[0] = renderGraph->GetTransientRenderTexture(workTextureHandles_[0]);
        workspace.workTextures[1] = renderGraph->GetTransientRenderTexture(workTextureHandles_[1]);
    }

    // CopyResource (mainRenderTex -> ppSrcTex)
    auto ppSrcTex = renderGraph->GetTransientRenderTexture(postUiSrcHandle_);
    
    // RenderGraph によるステート管理のため開始時に COPY_SOURCE に手動で遷移
    DirectXUtils::TransitionBarrier(cmdList, engine->GetMainRenderTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    cmdList->CopyResource(ppSrcTex->GetResource(), engine->GetMainRenderTexture()->GetResource());

    // バリア遷移
    DirectXUtils::TransitionBarrier(cmdList, ppSrcTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    renderGraph->RegisterResourceState(ppSrcTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    DirectXUtils::TransitionBarrier(cmdList, engine->GetMainRenderTexture()->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 最終出力先の決定
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
#ifdef EditorMode
    // EditorMode の場合、最終出力先は mainRenderTexture になる
    rtvHandle = engine->GetMainRenderTexture()->GetRtvHandle();
#else
    // 最終出力先はバックバッファ
    rtvHandle = drawManager->GetDxCommon()->GetRtvHandles(drawManager->GetDxCommon()->GetCurrentBackBufferIndex());
#endif

    ppMgr->Draw(cmdList, ppSrcTex, rtvHandle, workspace, PostProcessManager::Layer::PostUI);
}
