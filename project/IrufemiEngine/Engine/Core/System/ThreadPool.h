#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

/**
 * @class ThreadPool
 * @brief 複数のワーカースレッドを用いてタスクを並列実行するクラス
 */
class ThreadPool {
public:
    /**
     * @brief スレッドプールの初期化
     * @param numThreads 生成するスレッド数
     */
    explicit ThreadPool(size_t numThreads);
    
    /**
     * @brief デストラクタ（全スレッドの終了を待機）
     */
    ~ThreadPool();

    /**
     * @brief タスクをキューに追加し、戻り値を追跡するための future を返す
     * @tparam F 関数型
     * @tparam Args 引数型
     * @param f 実行する関数
     * @param args 関数の引数
     * @return 実行結果を取得するための std::future
     */
    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;

private:
    // ワーカースレッドの配列
    std::vector<std::thread> workers_;
    // タスクキュー
    std::queue<std::function<void()>> tasks_;
    
    // 同期用
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

inline ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    for(size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex_);
                    this->condition_.wait(lock, [this] { 
                        return this->stop_ || !this->tasks_.empty(); 
                    });
                    if(this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
        });
    }
}

template<class F, class... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result_t<F, Args...>> {
    using return_type = typename std::invoke_result_t<F, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if(stop_) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }
        tasks_.emplace([task]() { (*task)(); });
    }
    condition_.notify_one();
    return res;
}

inline ThreadPool::~ThreadPool() {  
    stop_ = true;
    condition_.notify_all();        
    for(std::thread &worker: workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
