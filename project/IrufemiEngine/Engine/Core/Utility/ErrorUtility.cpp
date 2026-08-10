#include "ErrorUtility.h"
#include "Log.h"
#include "StringUtility.h"
#include <iostream>

void ErrorUtility::ThrowIfFailed(HRESULT hr, const std::string& msg, const char* file, int line) {
    if (FAILED(hr)) {
        std::string errMsg = GetErrorMessage(hr);
        std::string fullMsg = "DirectX Error: " + errMsg;
        if (!msg.empty()) {
            fullMsg += " | Info: " + msg;
        }
        if (file) {
            fullMsg += "\nFile: " + std::string(file);
            fullMsg += "\nLine: " + std::to_string(line);
        }

        // ログ出力（コンソールやファイル）
        Log::OutPutLog(std::cerr, fullMsg);

        throw std::runtime_error(fullMsg);
    }
}

std::string ErrorUtility::GetErrorMessage(HRESULT hr) {
    _com_error err(hr);
#ifdef UNICODE
    return ConvertString(err.ErrorMessage());
#else
    return std::string(err.ErrorMessage());
#endif
}

void ErrorUtility::ShowErrorBox(const std::string& title, const std::string& message) {
    MessageBoxW(nullptr, ConvertString(message).c_str(), ConvertString(title).c_str(), MB_OK | MB_ICONERROR);
}

void ErrorUtility::Assert(bool condition, const std::string& msg, const char* file, int line) {
    if (!condition) {
        std::string fullMsg = "Assertion Failed!\n";
        if (!msg.empty()) fullMsg += "Message: " + msg + "\n";
        if (file) {
            fullMsg += "File: " + std::string(file) + "\n";
            fullMsg += "Line: " + std::to_string(line) + "\n";
        }
        
        // ログ出力
        Log::OutPutLog(std::cerr, fullMsg);

#ifdef _DEBUG
        // Debugビルド時はダイアログを出してからデバッガで停止
        int result = MessageBoxW(nullptr, ConvertString(fullMsg + "\nPress Abort to break into debugger, Retry to ignore, Ignore to continue.").c_str(), L"Irufemi Engine - Assertion Failed", MB_ABORTRETRYIGNORE | MB_ICONERROR);
        if (result == IDABORT) {
            __debugbreak();
        }
#else
        // Releaseビルド時は例外を投げて安全に終了
        throw std::runtime_error(fullMsg);
#endif
    }
}

void ErrorUtility::Warning(bool condition, const std::string& msg, const char* file, int line) {
    if (!condition) {
        std::string fullMsg = "[WARNING] ";
        if (!msg.empty()) fullMsg += msg + " ";
        if (file) {
            fullMsg += "(File: " + std::string(file) + ", Line: " + std::to_string(line) + ")\n";
        }
        
        // 警告ログ出力 (将来のコンソールにも送る)
        Log::OutPutLog(std::cerr, fullMsg);
        
        // 処理は停止せずそのまま続行
    }
}
