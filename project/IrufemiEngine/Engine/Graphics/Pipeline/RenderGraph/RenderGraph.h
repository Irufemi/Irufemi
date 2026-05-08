#pragma once
#include "IRenderPass.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <d3d12.h>
#include "RenderGraphBuilder.h"

class DrawManager;
class IrufemiEngine;

/**
 * @class RenderGraph
 * @brief 描画パス（RenderPass）を管理し、順次実行するクラス
 * @details 各パスの Setup を通じてリソースの使用状態を記録し、Execute 時に自動でリソースバリア（Transition）を発行します。
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

    /**
     * @brief リソースのステート追跡をリセットする（フレーム開始時などに呼ぶ）
     */
    void ResetStates();

    /**
     * @brief 特定のリソースの現在のステートを登録する（初期ステートの通知用）
     */
    void RegisterResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

private:
    std::vector<std::unique_ptr<IRenderPass>> passes_;
    std::unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> resourceStates_;
};
