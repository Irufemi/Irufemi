#pragma once
#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"

class MainOpaquePass : public IRenderPass {
public:
    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;
    void Execute(const Irufemi::RenderContext& rc) override;
};
