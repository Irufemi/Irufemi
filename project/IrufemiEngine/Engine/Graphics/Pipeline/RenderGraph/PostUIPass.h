#pragma once
#include "IRenderPass.h"
#include "RenderGraphBuilder.h"
#include <vector>
#include <array>

class PostUIPass : public IRenderPass {
public:
    PostUIPass() = default;
    ~PostUIPass() override = default;

    void Setup(class RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;

private:
    std::vector<TransientResourceHandle> workTextureHandles_;
    TransientResourceHandle postUiSrcHandle_ = kInvalidHandle;
};
