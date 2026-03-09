
#include "ParticleGPU.hlsli"
#include "RandomGenerator.hlsli"
#include "PerFrame.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);

RWStructuredBuffer<int32_t> gFreeCounter : register(u1);

ConstantBuffer<EmitterSphere> gEmitter : register(b0);

ConstantBuffer<PerFrame> gPerFrame : register(b1);

// 今回スレッド数は1。複数のEmitterを扱い、同時に処理したいような場合は適宜スレッド数を増やすと良い
[numthreads(1, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
	int32_t kMaxParticles = 1024;
	if (gEmitter.emit != 0) // 射出許可が出たので射出
	{
		// コンストラクタはないので、このように初期化すると良い。構造体的に初期化することは可能
		RandomGenerator generator;
		generator.seed = DTid.x * 12345 + uint3(1, 7, 11) * (uint) (gPerFrame.time * 1000.0f);
		// Generate3d呼ぶたびにseedが変わるので結果全ての乱数が変わる
		for (uint32_t countIndex = 0; countIndex < gEmitter.count; ++countIndex)
		{
			int32_t particleIndex;
			InterlockedAdd(gFreeCounter[0], 1, particleIndex); // gFreeCounter[0]に1を足し、足す前の値をparticleIndexに格納する
			// 最大よりもparticleの数が少なければ射出可能
			if (particleIndex < kMaxParticles)
			{
				// Particleの初期化
				gParticles[particleIndex].scale = generator.Generate3d();
				gParticles[particleIndex].translate = generator.Generate3d();
				gParticles[particleIndex].color.rgb = generator.Generate3d();
				gParticles[particleIndex].color.a = 1.0f;
				gParticles[particleIndex].velocity = (generator.Generate3d() * 2.0f - 1.0f) / 5.0f;
				gParticles[particleIndex].lifeTime = generator.Generate1d();
				gParticles[particleIndex].currentTime = 0.0f;
			}
		}
	}
	
	
}