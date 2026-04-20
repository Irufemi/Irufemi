#include "VoxelParticle.hlsli"
#include "PerFrame.hlsli"

RWStructuredBuffer<VoxelParticle> gParticles : register(u0);
ConstantBuffer<VoxelEmitter> gEmitter : register(b0);
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

    // 生きているパーティクルのみ更新
	if (gParticles[particleIndex].isActive == 1)
	{
		if (gParticles[particleIndex].life > 0.0f) {
			// 重力
			gParticles[particleIndex].velocity.y -= gEmitter.gravity * gPerFrame.deltaTime;
        
			// 収束
			float3 toCenter = gEmitter.emitPosition - gParticles[particleIndex].position;
			gParticles[particleIndex].velocity += toCenter * gEmitter.convergence * gPerFrame.deltaTime;

			// 位置更新
			gParticles[particleIndex].position += gParticles[particleIndex].velocity * gPerFrame.deltaTime;

            // ▼ 追加：Buildingタイプ(1)の場合は床(Y=0)で停止させる
            if (gEmitter.particleType == 1) {
                if (gParticles[particleIndex].position.y <= 0.0f) {
                    gParticles[particleIndex].position.y = 0.0f;
                    gParticles[particleIndex].velocity.y = 0.0f; // バウンドさせない
                    // 摩擦で横方向の速度を減衰
                    gParticles[particleIndex].velocity.x *= 0.5f;
                    gParticles[particleIndex].velocity.z *= 0.5f;
                }
            }

			// 生存時間更新
			gParticles[particleIndex].life -= (1.0f / gEmitter.lifeTime) * gPerFrame.deltaTime;
        
			// 色(アルファ)とサイズ更新
			gParticles[particleIndex].color.a = saturate(gParticles[particleIndex].life);
			gParticles[particleIndex].size = saturate(gParticles[particleIndex].life * 5.0f); // 最後の20%で縮小
		} else {
			// 寿命が尽きたら非アクティブにする
			gParticles[particleIndex].isActive = 0;
			gParticles[particleIndex].life = 0.0f;
			gParticles[particleIndex].color.a = 0.0f;
		}
	}
}