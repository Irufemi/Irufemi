#pragma once

#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"

/**
 * @class ComputePass
 * @brief RenderGraph の最初のパスとして、コンピュートシェーダを一括実行するパス
 */
class ComputePass : public IRenderPass {
public:
    ~ComputePass() override = default;

    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(class RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;
    void Execute(const Irufemi::RenderContext& rc) override;
};
