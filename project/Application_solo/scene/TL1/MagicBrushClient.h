#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <wrl.h>
#include <dxcapi.h>

class ShaderCompiler; // 前方宣言
class ShaderManager;

/**
 * @class MagicBrushClient
 * @brief Python側のローカルサーバーと通信し、AIシェーダーの生成および自己修復を管理する非同期クライアント
 */
class MagicBrushClient {
public:
    enum class State {
        Idle,
        Generating,
        Compiling,
        Fixing,
        Success,
        Error
    };

    MagicBrushClient();
    ~MagicBrushClient();

    /**
     * @brief シェーダー生成リクエストを非同期で開始する
     */
    void StartGeneration(const std::string& prompt, const std::string& referenceImagePath, const std::string& shaderName, const std::string& outputDirectory, ShaderManager* shaderManager);
    
    // サーバープロセス管理
    bool StartPythonServer();
    void StopPythonServer();
    void RestartPythonServer();
    bool IsServerRunning() const;

    /**
     * @brief 現在のステータスを取得する
     */
    State GetState() const { return state_.load(); }

    /**
     * @brief エラー時のメッセージを取得する
     */
    std::string GetErrorMessage() const;

    /**
     * @brief 成功時にコンパイル済みのシェーダーBlobを取得する
     */
    Microsoft::WRL::ComPtr<IDxcBlob> GetResultBlob();

private:
    void ProcessThread(std::string prompt, std::string referenceImagePath, std::string shaderName, std::string outputDirectory, ShaderManager* shaderManager);
    
    // HTTPリクエスト（curl.exe をプロセスとして呼び出す簡易実装）
    std::string SendPostRequest(const std::string& endpoint, const std::string& jsonPayload);
    
    // JSON用エスケープ
    std::string EscapeJSON(const std::string& input);

private:
    std::thread workerThread_;
    std::atomic<State> state_;
    
    mutable std::mutex mutex_;
    std::string errorMessage_;
    Microsoft::WRL::ComPtr<IDxcBlob> resultBlob_;
    
    // Pythonサーバープロセス用ハンドル
    void* pythonProcessHandle_ = nullptr; // HANDLE (Windows.hを含めないため void* にしておく)
    void* pythonThreadHandle_ = nullptr;
    unsigned long pythonProcessId_ = 0; // DWORD
    
    // 試行回数の上限
    const int32_t kMaxFixAttempts = 3;
};
