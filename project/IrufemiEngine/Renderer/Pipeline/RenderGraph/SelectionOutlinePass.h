#pragma once

#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"
#include <cstdint>

/**
 * @class SelectionOutlinePass
 * @brief 選択中のオブジェクトのマスクを描画し、シルエットのアウトラインを合成するパス
 */
class SelectionOutlinePass : public IRenderPass {
public:
    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;
    void Execute(const Irufemi::RenderContext& rc) override;

private:
    uint32_t maskHandle_ = static_cast<uint32_t>(-1);
};
