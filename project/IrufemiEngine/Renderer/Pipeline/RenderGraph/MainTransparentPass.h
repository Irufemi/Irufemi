#pragma once
#include "Renderer/Pipeline/RenderGraph/IRenderPass.h"
#include <cstdint>
#include <vector>

class MainTransparentPass : public IRenderPass {
public:
    /**
     * @brief 半透明描画パスで必要なリソース状態やバリアをレンダーグラフに登録する
     * @param[in,out] builder レンダーグラフビルダー
     * @param[in] drawManager 描画マネージャ
     * @param[in] engine エンジンコア
     */
    void Setup(RenderGraphBuilder& builder, const Irufemi::RenderContext& rc) override;
    void Execute(const Irufemi::RenderContext& rc) override;

private:
    /// @brief 半透明描画のインデックスソート用軽量キー (8 bytes)
    struct TransparentSortKey {
        float distanceToCamera;
        uint32_t packetIndex;
    };

    /// @brief 毎フレームのヒープアロケーションを防ぐための再利用ソートキーバッファ
    std::vector<TransparentSortKey> sortKeys_;
};
