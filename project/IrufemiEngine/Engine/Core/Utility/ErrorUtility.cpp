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
        OutputDebugStringW(ConvertString(fullMsg + "\n").c_str());

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
