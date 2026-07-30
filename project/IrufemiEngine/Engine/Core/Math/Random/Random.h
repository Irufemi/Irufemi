#pragma once
#include <random>
#include <numbers>


namespace Irufemi {
class Random {
private:

	//乱数生成エンジン
	static std::random_device seedGenerator_;
	//メルセンヌ・ツイスターエンジン(64bit版)
	static std::mt19937_64 randomEngine_;

public:

	static void SeedEngine();
	static float GeneratorFloat(float min, float max);
	static uint64_t GeneratorUint64(uint64_t min, uint64_t max);
};

} // namespace Irufemi
