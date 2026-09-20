#pragma once
#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"
#include "Renderer/Pipeline/RenderGraph/RenderGraphBuilder.h"
#include <vector>
#include <array>

class PostProcessPass : public IRenderPass {
public:
    PostProcessPass() = default;
    ~PostProcessPass() override = default;

    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(class RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;
    void Execute(const Irufemi::RenderContext& rc) override;

private:
    std::vector<TransientResourceHandle> workTextureHandles_;
    TransientResourceHandle bloomExtractHandle_ = kInvalidHandle;
    TransientResourceHandle bloomBlurHandle_ = kInvalidHandle;
    TransientResourceHandle lsExtractHandle_ = kInvalidHandle;
    TransientResourceHandle lsBlurHandle_ = kInvalidHandle;
    std::array<TransientResourceHandle, 8> kawaseTextureHandles_;

    TransientResourceHandle preUiSrcHandle_ = kInvalidHandle;
};
