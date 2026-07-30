#pragma once
#include "IRenderPass.h"
#include "RenderGraphBuilder.h"
#include <vector>
#include <array>

class PostUIPass : public IRenderPass {
public:
    PostUIPass() = default;
    ~PostUIPass() override = default;

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
    TransientResourceHandle postUiSrcHandle_ = kInvalidHandle;
};
