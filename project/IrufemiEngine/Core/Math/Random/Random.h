#pragma once
#include <random>
#include <numbers>

namespace Irufemi {
class Random {
private:
    // 各スレッド固有のメルセンヌ・ツイスターエンジン (64bit版)
    static thread_local std::mt19937_64 randomEngine_;

public:
    /**
     * @brief 現在のスレッドの乱数エンジンを再シードする
     */
    static void SeedEngine();
    /**
     * @brief 指定範囲の浮動小数点乱数を生成する
     */
    static float GeneratorFloat(float min, float max);
    /**
     * @brief 指定範囲の64bit整数乱数を生成する
     */
    static uint64_t GeneratorUint64(uint64_t min, uint64_t max);
};

} // namespace Irufemi
