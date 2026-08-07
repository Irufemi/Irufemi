#include "VoxelParticle.hlsli"
#include "PerFrame.hlsli"

RWStructuredBuffer<VoxelParticle> gParticles : register(u0);
StructuredBuffer<VoxelEmitter> gEmitters : register(t1);

cbuffer VoxelSystemCb : register(b0) {
    uint gVoxelCount;
    uint3 gPad;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint particleIndex = dispatchThreadID.x;

	// バッファ範囲チェック
	uint count, stride;
	gParticles.GetDimensions(count, stride);
	if (particleIndex >= count)
		return;

    uint emitterIndex = particleIndex / gVoxelCount;
    VoxelEmitter emitter = gEmitters[emitterIndex];

    // 生きているパーティクルのみ更新
	if (gParticles[particleIndex].isActive == 1)
	{
		if (gParticles[particleIndex].life > 0.0f) {
			if (gParticles[particleIndex].life <= 1.0f) {
				// 速度更新（重力・上昇気流）
				if (emitter.particleType == 2) {
					// 燃え尽きエフェクト
					gParticles[particleIndex].velocity.y -= (emitter.gravity * 0.6f) * gPerFrame.deltaTime;
					gParticles[particleIndex].velocity *= (1.0f - 1.5f * gPerFrame.deltaTime);
					
					float swayX = sin(gPerFrame.time * emitter.swayFrequency + particleIndex * 0.1f) * emitter.swayAmplitude;
					float swayZ = cos(gPerFrame.time * (emitter.swayFrequency * 0.8f) + particleIndex * 0.1f) * emitter.swayAmplitude;
					gParticles[particleIndex].velocity.x += swayX * gPerFrame.deltaTime;
					gParticles[particleIndex].velocity.z += swayZ * gPerFrame.deltaTime;

					float l = gParticles[particleIndex].life;
					if (l > 0.6f) {
						gParticles[particleIndex].color.rgb += emitter.startColor.rgb * gPerFrame.deltaTime;
					} else {
						gParticles[particleIndex].color.rgb -= emitter.endColor.rgb * gPerFrame.deltaTime;
						gParticles[particleIndex].color.rgb = max(float3(0.0f, 0.0f, 0.0f), gParticles[particleIndex].color.rgb);
					}
				} else if (emitter.particleType == 3) {
					// --- FineScatter ---
					gParticles[particleIndex].velocity *= (1.0f - 5.0f * gPerFrame.deltaTime);
					gParticles[particleIndex].velocity.y -= emitter.gravity * gPerFrame.deltaTime;
					
					float swayX = sin(gPerFrame.time * emitter.swayFrequency + particleIndex * 0.2f) * emitter.swayAmplitude;
					float swayZ = cos(gPerFrame.time * (emitter.swayFrequency * 0.9f) + particleIndex * 0.2f) * emitter.swayAmplitude;
					gParticles[particleIndex].velocity.x += swayX * gPerFrame.deltaTime;
					gParticles[particleIndex].velocity.z += swayZ * gPerFrame.deltaTime;

					float l = gParticles[particleIndex].life;
					if (l > 0.5f) {
						gParticles[particleIndex].color.rgb -= float3(5.0f, 15.0f, 20.0f) * gPerFrame.deltaTime; 
					} else {
						gParticles[particleIndex].color.rgb -= float3(20.0f, 10.0f, 5.0f) * gPerFrame.deltaTime; 
						gParticles[particleIndex].color.rgb = max(float3(0.0f, 0.0f, 0.0f), gParticles[particleIndex].color.rgb);
					}
				} else {
					gParticles[particleIndex].velocity.y -= emitter.gravity * gPerFrame.deltaTime;
				}
			
				// 収束
				float3 toCenter = emitter.emitPosition - gParticles[particleIndex].position;
				gParticles[particleIndex].velocity += toCenter * emitter.convergence * gPerFrame.deltaTime;

				// 位置更新
				gParticles[particleIndex].position += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
				
				// 回転更新
				gParticles[particleIndex].rotation += gParticles[particleIndex].angularVelocity * gPerFrame.deltaTime;
				
				// 空気抵抗による回転の減衰
				gParticles[particleIndex].angularVelocity *= (1.0f - 0.5f * gPerFrame.deltaTime);

				if (emitter.particleType == 1) {
					if (gParticles[particleIndex].position.y < 0.0f) {
						gParticles[particleIndex].position.y = 0.0f;
						gParticles[particleIndex].velocity = float3(0, 0, 0);
					}
				}
				else if (emitter.particleType == 4) { // DebrisLargeGravity
					if (gParticles[particleIndex].position.y < 0.0f) {
						gParticles[particleIndex].position.y = 0.0f;
						gParticles[particleIndex].velocity = float3(0, 0, 0);
						gParticles[particleIndex].angularVelocity = float3(0, 0, 0);
					}
					
					float lifeRatio = gParticles[particleIndex].life;
					if (lifeRatio < 0.8f) {
						gParticles[particleIndex].color.rgb += float3(0.05f, 0.01f, 0.0f);
					}
					if (lifeRatio < 0.4f) {
						gParticles[particleIndex].color.rgb *= 0.9f;
					}
				}
				else if (emitter.particleType == 5) { // DebrisExplosive
					gParticles[particleIndex].velocity *= 0.92f; 
					gParticles[particleIndex].size *= 0.95f; 
					
					float lifeRatio = gParticles[particleIndex].life;
					if (lifeRatio > 0.9f) {
						gParticles[particleIndex].color.rgb += emitter.startColor.rgb * gPerFrame.deltaTime;
					} else {
						gParticles[particleIndex].color.rgb *= 0.85f;
					}
				}
			}

			// 生存時間更新
			float currentLifeTime = max(0.1f, emitter.lifeTime);
			gParticles[particleIndex].life -= (1.0f / currentLifeTime) * gPerFrame.deltaTime;
        
			// 色(アルファ)とサイズ更新
			gParticles[particleIndex].color.a = saturate(gParticles[particleIndex].life);
			if (emitter.particleType == 2) {
				gParticles[particleIndex].size = saturate(gParticles[particleIndex].life * 1.5f);
			} else if (emitter.particleType == 3) {
				gParticles[particleIndex].size = 0.5f * saturate(gParticles[particleIndex].life * 4.0f);
			} else if (emitter.particleType == 5) {
			} else {
				gParticles[particleIndex].size = saturate(gParticles[particleIndex].life * 5.0f); 
			}
		} else {
			// 寿命が尽きたら非アクティブにする
			gParticles[particleIndex].isActive = 0;
			gParticles[particleIndex].life = 0.0f;
			gParticles[particleIndex].color.a = 0.0f;
		}
	}
}