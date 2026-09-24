#pragma once
#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"

/**
 * @class UIPass
 * @brief 通常UI（スプライト、テキスト等）を描画するパス
 */
class UIPass : public IRenderPass {
public:
    ~UIPass() override = default;

    /**
     * @brief パスのセットアップ処理
     * @param[in,out] builder リソースの使用状態を記録するビルダー
     * @param[in] rc 描画コンテキスト
     */
    void Setup(RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;

    /**
     * @brief パスの実行処理（UI描画コマンドの発行）
     * @param[in] rc 描画コンテキスト
     */
    void Execute(const Irufemi::RenderContext& rc) override;
};
