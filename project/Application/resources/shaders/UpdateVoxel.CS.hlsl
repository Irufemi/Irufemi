#include "VoxelParticle.hlsli"
#include "PerFrame.hlsli"

RWStructuredBuffer<VoxelParticle> gParticles : register(u0);
ConstantBuffer<VoxelEmitter> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint particleIndex = dispatchThreadID.x;

    // 生きているパーティクルのみ更新
	if (gParticles[particleIndex].life > 0.0f)
	{
        // 重力
		gParticles[particleIndex].velocity.y -= gEmitter.gravity * gPerFrame.deltaTime;
        
        // 収束
		float3 toCenter = gEmitter.emitPosition - gParticles[particleIndex].position;
		gParticles[particleIndex].velocity += toCenter * gEmitter.convergence * gPerFrame.deltaTime;

        // 位置更新
		gParticles[particleIndex].position += gParticles[particleIndex].velocity * gPerFrame.deltaTime;

        // 生存時間更新
		gParticles[particleIndex].life -= (1.0f / gEmitter.lifeTime) * gPerFrame.deltaTime;
        
        // 色(アルファ)更新
		gParticles[particleIndex].color.a = saturate(gParticles[particleIndex].life);
	}
}