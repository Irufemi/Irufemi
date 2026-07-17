#pragma once
#include <mutex>

namespace Irufemi {
    class AssimpMutex {
    public:
        static std::mutex& Get() {
            static std::mutex mutex;
            return mutex;
        }
    };
}
