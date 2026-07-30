#pragma once
#include "Engine/Core/Utility/ErrorUtility.h"
#include "Engine/Core/Utility/Log.h"
#include <iostream>
#include <d3d12.h>
#include <string>
#include <cassert>
#include <stdexcept>
#include <windows.h>

namespace Irufemi {
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
}

#define HR_CHECK(hr, msg) Irufemi::CheckHResult((hr), (msg), __FILE__, __LINE__)
