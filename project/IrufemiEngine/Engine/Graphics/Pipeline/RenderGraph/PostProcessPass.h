#pragma once
#include "IRenderPass.h"
#include "RenderGraphBuilder.h"
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
    void Setup(class RenderGraphBuilder& builder, class DrawManager* drawManager, class IrufemiEngine* engine) override;
    /**
     * @brief Execute を実行する。
     */
    void Execute(class DrawManager* drawManager, class IrufemiEngine* engine) override;

private:
    std::vector<TransientResourceHandle> workTextureHandles_;
    TransientResourceHandle bloomExtractHandle_ = kInvalidHandle;
    TransientResourceHandle bloomBlurHandle_ = kInvalidHandle;
    TransientResourceHandle lsExtractHandle_ = kInvalidHandle;
    TransientResourceHandle lsBlurHandle_ = kInvalidHandle;
    std::array<TransientResourceHandle, 8> kawaseTextureHandles_;

    TransientResourceHandle preUiSrcHandle_ = kInvalidHandle;
};
