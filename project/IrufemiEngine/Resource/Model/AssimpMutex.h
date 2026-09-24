#pragma once
#include <mutex>

namespace Irufemi {
class AssimpMutex {
public:
    /**
     * @brief Assimp の非スレッドセーフなパース処理を直列化するための排他制御ミューテックスを取得する
     * @return スレッド間で共有される排他制御ミューテックスの参照
     */
    static std::mutex& Get() {
        static std::mutex mutex;
        return mutex;
    }
};
} // namespace Irufemi
