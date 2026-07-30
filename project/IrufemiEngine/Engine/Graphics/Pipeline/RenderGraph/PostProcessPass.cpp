#include "PostProcessPass.h"
#include "Engine/Core/Math/Math.h"
#include "../../../Manager/DrawManager.h"
#include "../../../IrufemiEngine.h"
#include "../../PostProcess/PostProcessManager.h"
#include "../../DirectX/DirectXCommon.h"
#include "../../DirectX/DirectXUtils.h"
#include "RenderGraph.h"
#include <cstring>
void PostProcessPass::Setup(RenderGraphBuilder& builder, DrawManager* drawManager, IrufemiEngine* engine) {
    auto ppMgr = engine->GetPostProcessManager();
    const auto& activeModes = ppMgr->GetActiveModes();
    auto mainRenderTex = engine->GetMainRenderTexture();

    // 自身に書き戻すため、最終的な出力先として RENDER_TARGET を要求する
    builder.RequireState(mainRenderTex->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

    // G-Bufferをシェーダーリソースとして要求する
    if (auto tex = engine->GetEffectMaskTexture()) builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (auto tex = engine->GetNormalTexture()) builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (auto tex = engine->GetMaterialTexture()) builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (auto tex = engine->GetVelocityTexture()) builder.RequireState(tex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    workTextureHandles_.clear();
    bloomExtractHandle_ = kInvalidHandle;
    bloomBlurHandle_ = kInvalidHandle;
    kawaseTextureHandles_.fill(kInvalidHandle);

    D3D12_RESOURCE_DESC desc = mainRenderTex->GetResource()->GetDesc();

    // 入力ソース退避用テクスチャ (エフェクトの有無に関わらず常に必要)
    preUiSrcHandle_ = builder.CreateTransientResource("PP_PreUISrc", desc);
    builder.RequireTransientState(preUiSrcHandle_, D3D12_RESOURCE_STATE_COPY_DEST);

    if (!activeModes.empty()) {
        bool usesDepthBuffer = false;
        bool hasBloom = false;
        bool hasSeparableBlur = false;
        bool hasKawaseBlur = false;
        bool hasLightShafts = false;
        for (auto mode : activeModes) {
            if (mode == PostProcessMode::Bloom) hasBloom = true;
            if (PostProcessManager::UsesDepthBuffer(mode)) usesDepthBuffer = true;
            if (mode == PostProcessMode::Smoothing || mode == PostProcessMode::GaussianFilter) hasSeparableBlur = true;
            if (mode == PostProcessMode::DualKawaseBlur) hasKawaseBlur = true;
            if (mode == PostProcessMode::LightShafts) hasLightShafts = true;
        }

        if (usesDepthBuffer) {
            builder.RequireState(drawManager->GetDxCommon()->GetDepthStencilResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        
        // ピンポンバッファ用の一時テクスチャ (最大2枚)
        // ポストプロセスの中間計算はリニア空間で行うため、UNORM を指定する
        D3D12_RESOURCE_DESC workDesc = desc;
        workDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_Work0", workDesc));
        workTextureHandles_.push_back(builder.CreateTransientResource("PP_Work1", workDesc));

        if (hasBloom) {
            bloomExtractHandle_ = builder.CreateTransientResource("BloomExtract", workDesc);
        }
        if (hasBloom || hasSeparableBlur) {
            bloomBlurHandle_ = builder.CreateTransientResource("BloomBlur", workDesc);
        }
        if (hasLightShafts) {
            D3D12_RESOURCE_DESC lsDesc = workDesc;
            lsDesc.Width = (std::max<UINT64>)(1, lsDesc.Width / 2);
            lsDesc.Height = (std::max<UINT>)(1, lsDesc.Height / 2);
            lsExtractHandle_ = builder.CreateTransientResource("LS_Extract", lsDesc);
            lsBlurHandle_ = builder.CreateTransientResource("LS_Blur", lsDesc);
        }

        if (hasKawaseBlur) {
            D3D12_RESOURCE_DESC kwDesc = desc;
            kwDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            for (int i = 0; i < PostProcessManager::kMaxKawaseIterations; ++i) {
                kwDesc.Width = (std::max<UINT64>)(1, kwDesc.Width / 2);
                kwDesc.Height = (std::max<UINT>)(1, kwDesc.Height / 2);
                std::string name = "PP_Kawase_" + std::to_string(i);
                kawaseTextureHandles_[i] = builder.CreateTransientResource(name, kwDesc);
            }
        }

        // 初期ステートは全て SRV (内部で書き込み前に RTV に遷移させる)
        for (auto handle : workTextureHandles_) {
            builder.RequireTransientState(handle, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (hasBloom) {
            builder.RequireTransientState(bloomExtractHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (hasBloom || hasSeparableBlur) {
            builder.RequireTransientState(bloomBlurHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (hasLightShafts) {
            builder.RequireTransientState(lsExtractHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            builder.RequireTransientState(lsBlurHandle_, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        if (hasKawaseBlur) {
            for (int i = 0; i < PostProcessManager::kMaxKawaseIterations; ++i) {
                if (kawaseTextureHandles_[i] != kInvalidHandle) {
                    builder.RequireTransientState(kawaseTextureHandles_[i], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                }
            }
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
    if (lsExtractHandle_ != kInvalidHandle) {
        workspace.lsExtract = renderGraph->GetTransientRenderTexture(lsExtractHandle_);
    }
    if (lsBlurHandle_ != kInvalidHandle) {
        workspace.lsBlur = renderGraph->GetTransientRenderTexture(lsBlurHandle_);
    }
    for (int i = 0; i < PostProcessManager::kMaxKawaseIterations; ++i) {
        if (kawaseTextureHandles_[i] != kInvalidHandle) {
            workspace.kawaseTextures[i] = renderGraph->GetTransientRenderTexture(kawaseTextureHandles_[i]);
        } else {
            workspace.kawaseTextures[i] = nullptr;
        }
    }

    // 深度バッファを使用するエフェクトのための逆投影行列更新
    const auto& activeModes = ppMgr->GetActiveModes();
    bool needsProjectionInverse = false;
    for (auto mode : activeModes) {
        if (PostProcessManager::UsesDepthBuffer(mode)) {
            needsProjectionInverse = true;
        }
    }
    if (needsProjectionInverse) {
        if (auto* perFrameData = drawManager->GetPerFrameData()) {
            ppMgr->GetOutlineParams().projectionInverse = Irufemi::Math::Inverse(perFrameData->camera.projection);
        }
    }

    // CopyResource (mainRenderTex -> ppSrcTex)
    auto ppSrcTex = renderGraph->GetTransientRenderTexture(preUiSrcHandle_);
    
    // RenderGraph によるステート管理のため開始時に COPY_SOURCE に手動で遷移
    DirectXUtils::TransitionBarrier(cmdList, engine->GetMainRenderTexture()->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    cmdList->CopyResource(ppSrcTex->GetResource(), engine->GetMainRenderTexture()->GetResource());

    // バリア遷移
    DirectXUtils::TransitionBarrier(cmdList, ppSrcTex->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    renderGraph->SetInitialResourceState(ppSrcTex->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    DirectXUtils::TransitionBarrier(cmdList, engine->GetMainRenderTexture()->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 最終出力先は常に mainRenderTexture
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = engine->GetMainRenderTexture()->GetRtvHandle();
    ppMgr->Draw(cmdList, ppSrcTex, rtvHandle, workspace, PostProcessManager::Layer::PreUI);

    // 深度バッファを元の DEPTH_WRITE に戻す
    if (needsProjectionInverse) {
        DirectXUtils::TransitionBarrier(
            cmdList, drawManager->GetDxCommon()->GetDepthStencilResource(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
        );
        renderGraph->SetInitialResourceState(drawManager->GetDxCommon()->GetDepthStencilResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }
}
