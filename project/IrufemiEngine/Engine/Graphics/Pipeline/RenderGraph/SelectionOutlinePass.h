#pragma once

#include "IRenderPass.h"
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
    void Setup(RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    /**
     * @brief Execute を実行する。
     */
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;

private:
    uint32_t maskHandle_ = static_cast<uint32_t>(-1);
};
