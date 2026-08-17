#pragma once

#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"

/**
 * @class TopMostPass
 * @brief 最前面UI（デバッグ用テキストやトランジション等）を描画するパス
 * @details ポストプロセスのさらに後、バックバッファ（あるいは最終画面）に直接描画します。
 */
class TopMostPass : public IRenderPass {
public:
    ~TopMostPass() override = default;

    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(class RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    /**
     * @brief Execute を実行する。
     */
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
