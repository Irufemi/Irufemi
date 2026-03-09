
// classにしているがすべてpublic扱いになる。structとclassは完全に同じ。 publicやprivate自体がサポートされていない

//class RandomGenerator
//{
//	float32_t3 seed;
//	float32_t3 Generate3d()
//	{
//		seed = rand3dTo3d(seed);
//		return seed;
//	}
	
//	float32_t Generate1d()
//	{
//		float32_t result = rand3dTo1d(seed);
//		seed.x = result;
//		return result;
//	}
//};

// 上記の学校資料を基にXorshiftを使用するRandomGeneratorを作成する
class RandomGenerator
{
    // Xorshiftは整数(uint)で計算するのが基本
	uint3 seed;

    // 0.0f ～ 1.0f の範囲の乱数を3つ(xyz)生成する
	float3 Generate3d()
	{
        // Xorshiftのアルゴリズム（ビットをズラして混ぜる）
		seed ^= (seed << 13);
		seed ^= (seed >> 17);
		seed ^= (seed << 5);

        // uintの最大値(4294967295)で割って、0.0～1.0のfloatに変換
		return float3(seed) / 4294967295.0f;
	}
    
    // 0.0f ～ 1.0f の乱数を1つ生成する
	float Generate1d()
	{
        
		return Generate3d().x;

	}
};