#include "Core/Math/Random/Random.h"

namespace Irufemi {
// スレッドローカルな乱数生成エンジンの実体と初期化
thread_local std::mt19937_64 Random::randomEngine_{std::random_device{}()};

void Random::SeedEngine() {
    randomEngine_.seed(std::random_device{}());
}

float Random::GeneratorFloat(float min, float max) {
    std::uniform_real_distribution<float> distribution(min, max);

    // 乱数を返す
    return distribution(randomEngine_);
}

uint64_t Random::GeneratorUint64(uint64_t min, uint64_t max) {
    std::uniform_int_distribution<uint64_t> distribution(min, max);
    return distribution(randomEngine_);
}
} // namespace Irufemi
