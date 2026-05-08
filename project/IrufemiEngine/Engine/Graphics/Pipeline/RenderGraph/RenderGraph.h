#pragma once
#include "IRenderPass.h"
#include <vector>
#include <memory>

class DrawManager;
class IrufemiEngine;

/**
 * @class RenderGraph
 * @brief 描画パス（RenderPass）を管理し、順次実行するクラス
 * @details 登録された各パスの依存関係（現在は登録順）に基づいて Execute を呼び出します。
 *          将来的には Setup を用いたリソーストラッキングとバリア自動解決機能を持ちます。
 */
class RenderGraph {
public:
    RenderGraph() = default;
    ~RenderGraph() = default;

    /**
     * @brief 描画パスをグラフに登録する
     * @param[in] pass 登録する IRenderPass を実装したクラスのユニークポインタ
     */
    void AddPass(std::unique_ptr<IRenderPass> pass);

    /**
     * @brief 登録されたすべてのパスを順に実行する
     * @param[in] drawManager 描画コマンドを発行するための DrawManager
     * @param[in] engine エンジン本体
     */
    void Execute(DrawManager* drawManager, IrufemiEngine* engine);

    /**
     * @brief 登録されているすべてのパスを破棄する
     */
    void ClearPasses();

private:
    std::vector<std::unique_ptr<IRenderPass>> passes_;
};
