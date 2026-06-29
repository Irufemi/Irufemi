#pragma once

#include "IRenderPass.h"

/**
 * @class TopMostPass
 * @brief 最前面UI（デバッグ用テキストやトランジション等）を描画するパス
 * @details ポストプロセスのさらに後、バックバッファ（あるいは最終画面）に直接描画します。
 */
class TopMostPass : public IRenderPass {
public:
    ~TopMostPass() override = default;

    void Setup(class RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
