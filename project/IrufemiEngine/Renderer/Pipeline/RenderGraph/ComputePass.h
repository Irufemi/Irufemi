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
    void Setup(class RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override {}
    /**
     * @brief Execute を実行する。
     */
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
