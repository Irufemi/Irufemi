#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>  
#include <d3d12.h>
#include <wrl.h>
#include "Texture.h"
#include "../../../externals/DirectXTex/DirectXTex.h"
#include "../../Engine/Core/System/ThreadPool.h"
#include <atomic>
#include <future>
#include <type_traits>
#include <functional>

// 前方宣言
namespace DirectX {
    class ScratchImage;
}

class DirectXCommon;

/**
 * @class TextureManager
 * @brief テクスチャのロードと管理を一括して行うマネージャクラス
 * @details テクスチャの重複ロードを防ぐためのキャッシュ機構を持ち、IDによる指定でSRVハンドルを提供します。
 */
class TextureManager {
public:
    /**
     * @brief コンストラクタ
     */
    TextureManager() = default;

    /**
     * @brief デストラクタ
     */
    ~TextureManager() = default;

    /**
     * @brief 初期化
     * @param[in] dxCommon DirectX 12 基礎クラスのポインタ
     */
    void Initialize(DirectXCommon* dxCommon);

    /**
     * @brief 指定フォルダ内のすべての画像をロードする
     * @param[in] folderPath ロード対象のフォルダパス
     */
    void LoadAllFromFolder(const std::string& folderPath);

    /**
     * @brief テクスチャ名からGPU側のSRVハンドルを取得
     * @details 未ロードの場合はロードを試みます。
     * @param[in] name ファイルパスまたは識別名
     * @return GPU側のSRVハンドル
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetTextureHandle(const std::string& name) const;

    /**
     * @brief テクスチャ名からCPU側の画像データを取得
     * @param[in] name ファイルパスまたは識別名
     * @return ScratchImageへのポインタ
     */
    const DirectX::ScratchImage* GetScratchImage(const std::string& name) const;

    /**
     * @brief ロード済みのテクスチャ名一覧を取得
     */
    std::vector<std::string> GetTextureNames() const;

    /**
     * @brief フォールバック用のダミー白テクスチャを生成する
     */
    void CreateWhiteDummyTexture();

    /**
     * @brief テクスチャのピクセルサイズを取得
     * @param[in] name 識別名
     * @param[out] outWidth 幅の出力先
     * @param[out] outHeight 高さの出力先
     * @return 取得成功なら true
     */
    bool GetTextureSize(const std::string& name, uint32_t& outWidth, uint32_t& outHeight) const;

    /**
     * @brief 白テクスチャのGPUハンドルを取得
     */
    D3D12_GPU_DESCRIPTOR_HANDLE GetWhiteTextureHandle() const { return whiteTextureHandle_; }

    /**
     * @brief すべてのロードタスクが完了したかを取得
     */
    bool IsAllLoaded() const { return pendingTaskCount_.load() == 0; }

    /**
     * @brief 非同期タスクの実行
     */
    template <class F, class... Args>
    std::future<void> EnqueueTask(F&& f, Args&&... args) {
        pendingTaskCount_++;
        auto task = std::make_shared<std::packaged_task<void()>>(
            std::bind(
                [this](auto&& func, auto&&... params) mutable {
                struct CountGuard {
                    std::atomic<uint32_t>& count;
                    ~CountGuard() { count--; }
                } guard{ pendingTaskCount_ };
                std::invoke(std::move(func), std::move(params)...);
            },
                std::forward<F>(f), std::forward<Args>(args)...
                )
        );
        std::future<void> res = task->get_future();
        threadPool_->Enqueue([task]() { (*task)(); });
        return res;
    }

    /**
     * @brief 白テクスチャのリソースを取得
     */
    ID3D12Resource* GetWhiteTextureResource() const { return whiteTextureResource_.Get(); }

private:
    DirectXCommon* dxCommon_ = nullptr;

    // key: ファイルパス(または識別名)、value: Texture オブジェクト
    mutable std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    mutable std::mutex mutex_;

    std::unique_ptr<ThreadPool> threadPool_;
    std::atomic<uint32_t> pendingTaskCount_{ 0 };

    // フォールバック白テクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource> whiteTextureResource_;
    D3D12_GPU_DESCRIPTOR_HANDLE whiteTextureHandle_{ 0 };

};