#include "RenderGraph.h"

void RenderGraph::AddPass(std::unique_ptr<IRenderPass> pass) {
    if (pass) {
        passes_.push_back(std::move(pass));
    }
}

void RenderGraph::Execute(DrawManager* drawManager, IrufemiEngine* engine) {
    for (auto& pass : passes_) {
        // 将来的にここで Setup() から得た情報に基づく自動バリア解決処理を追加可能
        pass->Execute(drawManager, engine);
    }
}

void RenderGraph::ClearPasses() {
    passes_.clear();
}
