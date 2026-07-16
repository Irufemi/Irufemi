#pragma once
#include <string>

//namespace StringUtility
//{

/*ログを出そう*/

std::wstring ConvertString(const std::string& str);

std::string ConvertString(const std::wstring& str);

namespace StringUtility {
    bool EndsWith(const std::wstring& str, const std::wstring& suffix);
    std::string GetCacheFilePath(const std::string& fullPath, const std::string& cacheCategory, const std::string& extension);
}

//};

