#pragma once
#include <mutex>

namespace Irufemi {
class AssimpMutex {
public:
    /**
     * @brief  を取得する。
     * @return 取得された
     */
    static std::mutex& Get() {
        static std::mutex mutex;
        return mutex;
    }
};
} // namespace Irufemi
