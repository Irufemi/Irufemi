#include "RenderGraph.h"
#include "../../../Manager/DrawManager.h"
#include "../../DirectX/DirectXCommon.h"

void RenderGraph::AddPass(std::unique_ptr<IRenderPass> pass) {
    if (pass) {
        passes_.push_back(std::move(pass));
    }
}

void RenderGraph::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    auto* cmdList = drawManager->GetDxCommon()->GetCommandList();

    for (auto& pass : passes_) {
        RenderGraphBuilder builder;
        pass->Setup(builder, drawManager, engine);

        // バリアの解決
        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        for (const auto& usage : builder.GetUsages()) {
            if (!usage.resource) continue;

            auto it = resourceStates_.find(usage.resource);
            D3D12_RESOURCE_STATES currentState = (it != resourceStates_.end()) ? it->second : D3D12_RESOURCE_STATE_COMMON;

            if (currentState != usage.state) {
                D3D12_RESOURCE_BARRIER b{};
                b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                b.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                b.Transition.pResource = usage.resource;
                b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                b.Transition.StateBefore = currentState;
                b.Transition.StateAfter = usage.state;
                barriers.push_back(b);

                resourceStates_[usage.resource] = usage.state;
            }
        }

        // バリアの発行
        if (!barriers.empty()) {
            cmdList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
        }

        // 実際の描画コマンドの積み込み
        pass->Execute(drawManager, engine);
    }
}

void RenderGraph::ClearPasses() {
    passes_.clear();
}

void RenderGraph::ResetStates() {
    resourceStates_.clear();
}

void RenderGraph::RegisterResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) {
    if (resource) {
        resourceStates_[resource] = state;
    }
}
