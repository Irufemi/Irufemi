#pragma once
#include <windows.h>
#include <string>
#include <stdexcept>
#include <comdef.h>

class ErrorUtility {
public:
    /**
     * @brief HRESULTをチェックし、FAILEDであれば例外をスローする
     * @param hr チェックするHRESULT
     * @param msg 任意のエラーメッセージ
     * @param file ファイル名 (__FILE__)
     * @param line 行番号 (__LINE__)
     */
    static void ThrowIfFailed(HRESULT hr, const std::string& msg = "", const char* file = nullptr, int line = 0);

    /**
     * @brief HRESULTのエラーコードから詳細なエラーメッセージ文字列を生成する
     */
    static std::string GetErrorMessage(HRESULT hr);

    /**
     * @brief メッセージボックスでエラーを通知する
     */
    static void ShowErrorBox(const std::string& title, const std::string& message);

    /**
     * @brief 致命的なエラーアサーション
     */
    static void Assert(bool condition, const std::string& msg, const char* file, int line);

    /**
     * @brief 非致命的な警告アサーション
     */
    static void Warning(bool condition, const std::string& msg, const char* file, int line);
};

// HRESULTエラーハンドリング
#define ASSERT_IF_FAILED(hr) ErrorUtility::ThrowIfFailed(hr, "", __FILE__, __LINE__)
#define ASSERT_IF_FAILED_MSG(hr, msg) ErrorUtility::ThrowIfFailed(hr, msg, __FILE__, __LINE__)

// エンジン独自の標準アサーション
#define IRUFEMI_ASSERT(condition) ErrorUtility::Assert(!!(condition), #condition, __FILE__, __LINE__)
#define IRUFEMI_ASSERT_MSG(condition, msg) ErrorUtility::Assert(!!(condition), msg, __FILE__, __LINE__)
#define IRUFEMI_WARNING(condition, msg) ErrorUtility::Warning(!!(condition), msg, __FILE__, __LINE__)

