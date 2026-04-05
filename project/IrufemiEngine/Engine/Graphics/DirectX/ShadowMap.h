#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class DirectXCommon;

/**
 * @class ShadowMap
 * @brief シャドウマッピング用の深度バッファを管理するクラス
 * @details 深度書き込み用の DSV と、シェーダー読み込み用の SRV を保持し、
 *          ライト視点でのレンダリングターゲットとして機能します。
 */
class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();

    /**
     * @brief 初期化
     * @param dxCommon DirectX基盤へのポインタ
     * @param width シャドウマップの幅
     * @param height シャドウマップの高さ
     */
    void Initialize(DirectXCommon* dxCommon, uint32_t width, uint32_t height);

    /**
     * @brief 描画開始処理（DSV のセットとビューポート設定）
     * @param commandList コマンドリスト
     */
    void BeginRender(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief 深度バッファのクリア
     * @param commandList コマンドリスト
     */
    void Clear(ID3D12GraphicsCommandList* commandList);

    /** @name ゲッター */
    ///@{
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return srvHandleGPU_; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle() const { return dsvHandleCPU_; }
    ID3D12Resource* GetResource() const { return resource_.Get(); }
    ///@}

private:
    DirectXCommon* dxCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
    
    // DSV 関連
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU_{};
    uint32_t dsvIndex_ = 0xFFFFFFFF;

    // SRV 関連
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU_{};
    uint32_t srvIndex_ = 0xFFFFFFFF;

    // ビューポート・シザーレクト
    D3D12_VIEWPORT viewport_{};
    D3D12_RECT scissorRect_{};
};
