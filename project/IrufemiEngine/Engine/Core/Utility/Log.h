#pragma once  

#include <fstream> 
#include <ostream>  
#include <string>  

#include <vector>

class Log
{
public:
    struct LogEntry {
        std::string message;
        bool isError;
    };

private: // メンバ変数  
    std::ofstream logStream;
    inline static std::vector<LogEntry> logHistory_; // ログの履歴バッファ
    static const size_t MAX_LOG_LINES = 1000;    // メモリ保護のための最大行数

public: // メンバ関数  
    /// <summary>  
    /// 初期化  
    /// </summary>  
    void Initialize();

    // ゲッター  
    std::ofstream& GetLogStream() { return logStream; }

    // 出力ウィンドウに文字を出す  
    static void OutPutLog(std::ostream& os, const std::string& message);

    /**
     * @brief 現在のログ履歴を取得します（エディタのコンソールパネル用）
     */
    static const std::vector<LogEntry>& GetLogHistory() { return logHistory_; }

    /**
     * @brief ログ履歴をクリアします
     */
    static void ClearLogHistory() { logHistory_.clear(); }
};