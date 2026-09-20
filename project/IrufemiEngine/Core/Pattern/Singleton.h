#pragma once
#include "Core/Utility/ErrorUtility.h"

#include <memory>
#include <cassert>

/**
 * @class Singleton
 * @brief ゲーム開発向けの安全な手動ライフサイクル管理型シングルトンベースクラス (CRTP)
 * @details 派生クラスは friend class Singleton<T>; を指定し、コンストラクタ/デストラクタを private
 * にする必要があります。 初期化順序のバグを防ぐため、必ずメインスレッドの適切なタイミングで Initialize()
 * を呼び出してください。
 */
template <typename T> class Singleton {
protected:
    Singleton() = default;
    ~Singleton() = default;

public:
    // コピーとムーブを禁止
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    /**
     * @brief インスタンスを生成する
     * @details メインループ開始前（DirectX初期化後など）に手動で呼び出します。
     */
    static void Initialize() {
        IRUFEMI_ASSERT(!instance_ && "Singleton is already initialized.");
        instance_ = std::unique_ptr<T>(new T());
    }

    /**
     * @brief インスタンスを破棄する
     * @details メインループ終了後（DirectX破棄前など）に手動で呼び出し、確実にリソースを解放します。
     */
    static void Finalize() {
        instance_.reset();
    }

    /**
     * @brief インスタンスを取得する
     * @return シングルトンインスタンスのポインタ
     */
    static T* GetInstance() {
        IRUFEMI_ASSERT(instance_ && "Singleton is not initialized. Call Initialize() first.");
        return instance_.get();
    }

    /**
     * @brief インスタンスが初期化されているか判定する
     */
    static bool IsInitialized() {
        return instance_ != nullptr;
    }

private:
    static inline std::unique_ptr<T> instance_{nullptr};
};
