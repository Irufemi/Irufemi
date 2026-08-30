#pragma once

#include <fstream>
#include <ostream>
#include <string>

#include <mutex>
#include <vector>

class Log {
public:
    struct LogEntry {
        std::string message;
        bool isError;
    };

private: // メンバ変数
    std::ofstream logStream;
    inline static std::vector<LogEntry> logHistory_; // ログの履歴バッファ
    inline static std::mutex logMutex_;              // ログ履歴保護用ミューテックス
    static const size_t MAX_LOG_LINES = 1000;        // メモリ保護のための最大行数

public: // メンバ関数
    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    // ゲッター
    /**
     * @brief LogStream を取得する。
     * @return 取得された LogStream
     */
    std::ofstream& GetLogStream() {
        return logStream;
    }

    // 出力ウィンドウに文字を出す
    /**
     * @brief OutPutLog を実行する。
     */
    static void OutPutLog(std::ostream& os, const std::string& message);

    /**
     * @brief 現在のログ履歴を取得します（エディタのコンソールパネル用）
     */
    static std::vector<LogEntry> GetLogHistory() {
        std::lock_guard<std::mutex> lock(logMutex_);
        return logHistory_;
    }

    /**
     * @brief ログ履歴をクリアします
     */
    static void ClearLogHistory() {
        std::lock_guard<std::mutex> lock(logMutex_);
        logHistory_.clear();
    }
};