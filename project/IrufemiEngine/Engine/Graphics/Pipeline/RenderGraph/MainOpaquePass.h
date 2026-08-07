#pragma once
#include "IRenderPass.h"

class MainOpaquePass : public IRenderPass {
public:
    /**
     * @brief up を設定する。
     * @param[in] builder 設定する up の値
     * @param[in] drawManager 設定する up の値
     * @param[in] engine 設定する up の値
     */
    void Setup(RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    /**
     * @brief Execute を実行する。
     */
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;
};
