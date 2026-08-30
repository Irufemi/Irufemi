#pragma once
#include "Core/Utility/ErrorUtility.h"
#include "Core/Utility/Log.h"
#include <cassert>
#include <d3d12.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace Irufemi {
/**
 * @brief CheckHResult を実行する。
 */
inline void CheckHResult(HRESULT hr, const char* msg, const char* file, int line) {
    if (FAILED(hr)) {
        std::string errorMsg = std::string("DirectX Error! [") + msg + "]\n";
        errorMsg += "File: " + std::string(file) + "\n";
        errorMsg += "Line: " + std::to_string(line) + "\n";

        char hrStr[32];
        sprintf_s(hrStr, "HRESULT: 0x%08X\n", (unsigned int)hr);
        errorMsg += hrStr;

        Log::OutPutLog(std::cerr, errorMsg);
        IRUFEMI_ASSERT(false && "DirectX API call failed. Check the log for details.");
        throw std::runtime_error(errorMsg);
    }
}
} // namespace Irufemi

#define HR_CHECK(hr, msg) Irufemi::CheckHResult((hr), (msg), __FILE__, __LINE__)
