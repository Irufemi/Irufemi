#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <functional>

// 前方宣言
class DirectXCommon;

/**
 * @class DXCommandManager
 * @brief DirectX12のコマンドキュー、コマンドリスト、フェンスの生成と管理を行うクラス
 */
class DXCommandManager {
public:
    DXCommandManager() = default;
    ~DXCommandManager() = default;

    /**
     * @brief 初期化
     * @param[in] device D3D12デバイス
     */
    void Initialize(ID3D12Device* device);

    /**
     * @brief 解放処理
     */
    void Finalize();

    /**
     * @brief GPUの全処理が完了するのを同期待機する
     */
    void WaitForGPU();

    /**
     * @brief 任意のコマンドリストを同期的にアップロードキューで実行し待機します
     * @param[in] commands コマンドを記録する関数（ラムダ等）
     */
    void ExecuteUploadCommands(std::function<void(ID3D12GraphicsCommandList*)> commands);

public: // ゲッター・セッター
    /**
     * @brief CommandQueue を取得する。
     * @return 取得された CommandQueue
     */
    ID3D12CommandQueue* GetCommandQueue() const {
        return commandQueue_.Get();
    }
    /**
     * @brief CommandAllocator を取得する。
     * @return 取得された CommandAllocator
     */
    ID3D12CommandAllocator* GetCommandAllocator(uint32_t frameIndex) const;
    /**
     * @brief CommandList を取得する。
     * @return 取得された CommandList
     */
    ID3D12GraphicsCommandList* GetCommandList() const {
        return commandList_.Get();
    }

    /**
     * @brief Fence を取得する。
     * @return 取得された Fence
     */
    ID3D12Fence* GetFence() const {
        return fence_.Get();
    }
    /**
     * @brief FenceEvent を取得する。
     * @return 取得された FenceEvent
     */
    HANDLE GetFenceEvent() const {
        return fenceEvent_;
    }

    /**
     * @brief FenceValue を取得する。
     * @return 取得された FenceValue
     */
    uint64_t& GetFenceValue(uint32_t frameIndex);
    /**
     * @brief FenceValue を取得する。
     * @return 取得された FenceValue
     */
    uint64_t GetFenceValue(uint32_t frameIndex) const;

    /**
     * @brief GlobalFenceValue を取得する。
     * @return 取得された GlobalFenceValue
     */
    uint64_t GetGlobalFenceValue() const {
        return globalFenceValue_;
    }
    /**
     * @brief IncrementGlobalFence を実行する。
     */
    uint64_t IncrementGlobalFence() {
        return ++globalFenceValue_;
    }

    /**
     * @brief UploadCommandAllocator を取得する。
     * @return 取得された UploadCommandAllocator
     */
    ID3D12CommandAllocator* GetUploadCommandAllocator() const {
        return uploadCommandAllocator_.Get();
    }
    /**
     * @brief UploadCommandList を取得する。
     * @return 取得された UploadCommandList
     */
    ID3D12GraphicsCommandList* GetUploadCommandList() const {
        return uploadCommandList_.Get();
    }
    /**
     * @brief UploadFence を取得する。
     * @return 取得された UploadFence
     */
    ID3D12Fence* GetUploadFence() const {
        return uploadFence_.Get();
    }
    /**
     * @brief UploadFenceValue を取得する。
     * @return 取得された UploadFenceValue
     */
    uint64_t GetUploadFenceValue() const {
        return uploadFenceValue_;
    }
    /**
     * @brief IncrementUploadFenceValue を実行する。
     */
    void IncrementUploadFenceValue() {
        uploadFenceValue_++;
    }

    /**
     * @brief UploadMutex を取得する。
     * @return 取得された UploadMutex
     */
    std::mutex& GetUploadMutex() {
        return uploadMutex_;
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Device> device_ = nullptr;

    // --- メインコマンド系 ---
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_ = nullptr;
    std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> commandAllocators_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_ = nullptr;

    // --- 同期・フェンス系 ---
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
    std::vector<uint64_t> fenceValues_;
    std::atomic<uint64_t> globalFenceValue_{0};
    HANDLE fenceEvent_ = nullptr;

    // --- 非同期転送（アップロード）系 ---
    std::mutex uploadMutex_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> uploadCommandAllocator_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> uploadCommandList_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Fence> uploadFence_ = nullptr;
    uint64_t uploadFenceValue_ = 0;
};
