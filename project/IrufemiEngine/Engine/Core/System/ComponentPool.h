#pragma once
#include <memory>
#include <vector>
#include <array>
#include <mutex>
#include <functional>
#include <cstddef>

/**
 * @class ComponentPool
 * @brief コンポーネントを連続したメモリ領域に配置する（DOD）ためのチャンクベース・メモリプール
 * @tparam T 対象となるコンポーネントの型
 * @tparam ChunkSize 1つのチャンク（ブロック）に格納する要素数。デフォルトは1024。
 */
template<typename T, size_t ChunkSize = 1024>
class ComponentPool {
public:
    static ComponentPool& GetInstance() {
        static ComponentPool instance;
        return instance;
    }

    /**
     * @brief プールからメモリを確保し、コンポーネントを生成する
     * @return 確保されたコンポーネントの shared_ptr（カスタムデリータ付き）
     */
    template<typename... Args>
    std::shared_ptr<T> Create(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 既存のチャンクから空きスロットを探す
        for (size_t chunkIdx = 0; chunkIdx < chunks_.size(); ++chunkIdx) {
            auto& chunk = chunks_[chunkIdx];
            for (size_t i = 0; i < ChunkSize; ++i) {
                if (!chunk->active[i]) {
                    chunk->active[i] = true;
                    // Placement new を使用して事前確保されたメモリ領域にオブジェクトを構築
                    T* ptr = new (&chunk->data[i]) T(std::forward<Args>(args)...);
                    
                    // shared_ptrが破棄される際に呼ばれるカスタムデリータ
                    return std::shared_ptr<T>(ptr, [chunkIdx, i](T* p) {
                        ComponentPool::GetInstance().Free(chunkIdx, i, p);
                    });
                }
            }
        }
        
        // 全チャンクが埋まっていれば新しいチャンクを追加
        auto newChunk = std::make_unique<Chunk>();
        newChunk->active[0] = true;
        T* ptr = new (&newChunk->data[0]) T(std::forward<Args>(args)...);
        
        size_t newChunkIdx = chunks_.size();
        chunks_.push_back(std::move(newChunk));
        
        return std::shared_ptr<T>(ptr, [newChunkIdx](T* p) {
            ComponentPool::GetInstance().Free(newChunkIdx, 0, p);
        });
    }

    /**
     * @brief 確保されている全てのコンポーネントに対して一括で処理を行う（DODのコア）
     * @details メモリが連続しているため、CPUキャッシュヒット率が劇的に向上します
     */
    template<typename Func>
    void ForEach(Func f) {
        // パフォーマンス最優先のためロックを取らずに実行します（Update専用）
        for (auto& chunk : chunks_) {
            for (size_t i = 0; i < ChunkSize; ++i) {
                if (chunk->active[i]) {
                    f(*reinterpret_cast<T*>(&chunk->data[i]));
                }
            }
        }
    }

private:
    ComponentPool() = default;
    ~ComponentPool() {
        // プール破棄時の処理（念のため強制的にデストラクタを呼ぶ）
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& chunk : chunks_) {
            for (size_t i = 0; i < ChunkSize; ++i) {
                if (chunk->active[i]) {
                    reinterpret_cast<T*>(&chunk->data[i])->~T();
                }
            }
        }
    }

    ComponentPool(const ComponentPool&) = delete;
    ComponentPool& operator=(const ComponentPool&) = delete;

    /**
     * @brief shared_ptrの参照カウントが0になった時に呼ばれ、メモリを解放状態にする
     */
    void Free(size_t chunkIdx, size_t itemIdx, T* p) {
        std::lock_guard<std::mutex> lock(mutex_);
        p->~T(); // デストラクタを明示的に呼び出す
        chunks_[chunkIdx]->active[itemIdx] = false;
    }

    struct Chunk {
        // std::vectorだと再配置でポインタが無効化されるため、
        // Placement new 用の固定サイズ・未初期化バイト配列（アライメント指定）を使用
        alignas(T) std::byte data[ChunkSize][sizeof(T)];
        bool active[ChunkSize] = {false};
    };

    std::vector<std::unique_ptr<Chunk>> chunks_;
    std::mutex mutex_;
};

/**
 * @brief 指定したコンポーネントがComponentPoolを使用するかどうかを判定するトレイト
 * @details プール対応させたいコンポーネントのヘッダで、この構造体を特殊化（std::true_type）してください。
 */
template <typename T>
struct IsPooledComponent : std::false_type {};
