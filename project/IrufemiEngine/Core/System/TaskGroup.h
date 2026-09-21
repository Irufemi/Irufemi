#pragma once
#include <atomic>
#include <cstdint>

/**
 * @class TaskGroup
 * @brief 非同期タスクのグループ進捗（残数）を管理するクラス
 */
class TaskGroup {
public:
    TaskGroup() : pendingCount_(0) {}
    ~TaskGroup() = default;

    /**
     * @brief タスクの開始を通知（カウントアップ）
     */
    void NotifyTaskStarted() {
        pendingCount_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @brief タスクの完了を通知（カウントダウン）
     */
    void NotifyTaskFinished() {
        uint32_t current = pendingCount_.load(std::memory_order_relaxed);
        while (current > 0 && !pendingCount_.compare_exchange_weak(
                                  current, current - 1,
                                  std::memory_order_release,
                                  std::memory_order_relaxed)) {
            // CASループにより、0未満へのアンダーフロー（UINT32_MAX化）を防止
        }
    }

    /**
     * @brief 全てのタスクが完了したか確認
     * @return true: 全完了, false: 未完了タスクあり
     */
    bool IsAllDone() const {
        return pendingCount_.load(std::memory_order_acquire) == 0;
    }

    /**
     * @brief 現在の待機中タスク数を取得
     */
    uint32_t GetPendingCount() const {
        return pendingCount_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint32_t> pendingCount_;
};
