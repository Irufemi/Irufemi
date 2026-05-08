#pragma once
#include <d3d12.h>
#include <vector>

/**
 * @class RenderGraphBuilder
 * @brief 各描画パスが必要とするリソース（テクスチャやレンダーターゲット等）の状態を登録するためのビルダークラス
 */
class RenderGraphBuilder {
public:
    struct ResourceUsage {
        ID3D12Resource* resource;
        D3D12_RESOURCE_STATES state;
    };

    /**
     * @brief 特定のリソースがこのパスの実行時に指定したステートであることを要求する
     * @param resource 必須ステートを要求する対象のリソース
     * @param state 要求するステート
     */
    void RequireState(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) {
        if (resource) {
            usages_.push_back({ resource, state });
        }
    }

    const std::vector<ResourceUsage>& GetUsages() const { return usages_; }
    void Clear() { usages_.clear(); }

private:
    std::vector<ResourceUsage> usages_;
};
