#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

class DirectXCommon;

/**
 * @class GpuProfiler
 * @brief DirectX12のタイムスタンプクエリを用いて、GPUの描画にかかった時間を計測するクラス
 */
class GpuProfiler {
public:
    static GpuProfiler& GetInstance() {
        static GpuProfiler instance;
        return instance;
    }

    /**
     * @brief 初期化処理。クエリヒープと結果格納用バッファを作成する
     * @param dxCommon DirectX共通基盤
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief コマンドリストの先頭（描画開始時）で呼び出し、開始タイムスタンプを記録する
     */
    void StartFrame(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief コマンドリストの末尾（描画終了時）で呼び出し、終了タイムスタンプを記録し、
     *        バッファへの解決（Resolve）を行う
     */
    void EndFrame(ID3D12GraphicsCommandList* commandList);

    /**
     * @brief 直前に取得完了したGPUフレーム時間（ミリ秒）を返す
     */
    float GetLastFrameGpuTimeMs() const { return lastGpuTimeMs_; }

private:
    GpuProfiler() = default;
    ~GpuProfiler() = default;

    // コピー禁止
    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

private:
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> queryResultBuffer_;

    DirectXCommon* dxCommon_ = nullptr;
    
    uint64_t gpuFrequency_ = 0;
    float lastGpuTimeMs_ = 0.0f;
    bool isInitialized_ = false;

    static const uint32_t kMaxFramesInFlight = 3;
};
