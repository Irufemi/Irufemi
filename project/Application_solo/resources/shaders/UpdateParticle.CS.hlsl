#include "ParticleGPU.hlsli"
#include "RandomGenerator.hlsli"
#include "PerFrame.hlsli"

static const uint kMaxParticles = 32768;

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<int> gFreeList : register(u2);
ConstantBuffer<GPUParticleEmitter> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticles[particleIndex].currentTime < gParticles[particleIndex].lifeTime)
        {
            float dt = gPerFrame.deltaTime;
            
            // 座標のゆらぎ (Jitter)
            if (gEmitter.jitter > 0.0f) {
                RandomGenerator rng;
                rng.seed = uint3(particleIndex, (uint)gPerFrame.time, (uint)gPerFrame.time + 100);
                float3 randomVal = rng.Generate3d() * 2.0f - 1.0f;
                gParticles[particleIndex].translate += randomVal * gEmitter.jitter;
            }

            // 進捗 (0.0: 生まれたて, 1.0: 寿命)
            float t = saturate(gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
            
            // アトラクター (引力)
            if (abs(gEmitter.attractorStrength) > 0.0001f) {
                float3 dir = gEmitter.attractorPos - gParticles[particleIndex].translate;
                float distSq = max(dot(dir, dir), 0.01f);
                float3 force = normalize(dir) * (gEmitter.attractorStrength / distSq);
                gParticles[particleIndex].velocity += force * dt;
            }

            // 物理更新: 重力と空気抵抗
            gParticles[particleIndex].velocity.y -= gEmitter.gravity * dt;
            gParticles[particleIndex].velocity *= pow(saturate(1.0f - gEmitter.damping), dt * 60.0f); // 60fps基準の減衰

            // 移動更新
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity;
            
            // 床衝突判定
            if (gParticles[particleIndex].translate.y < gEmitter.groundHeight) {
                gParticles[particleIndex].translate.y = gEmitter.groundHeight;
                gParticles[particleIndex].velocity.y *= -gEmitter.bounce;
                // XZ平面の摩擦（簡易的に減衰）
                gParticles[particleIndex].velocity.xz *= 0.8f;
            }

            gParticles[particleIndex].currentTime += dt;

            // 回転更新 (自転)
            gParticles[particleIndex].rotation += gParticles[particleIndex].rotateSpeed * dt;

            // カラー更新: Start -> End Lerp
            gParticles[particleIndex].color = lerp(gParticles[particleIndex].startColor, gParticles[particleIndex].endColor, t);
            
            // スケール更新: Start -> End Lerp
            gParticles[particleIndex].scale = lerp(gParticles[particleIndex].startScale, gParticles[particleIndex].endScale, t);

            // 寿命終了判定
            if (gParticles[particleIndex].currentTime >= gParticles[particleIndex].lifeTime)
            {
                gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                gParticles[particleIndex].color.a = 0.0f;

                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                if ((freeListIndex + 1) >= 0 && (freeListIndex + 1) < (int)kMaxParticles)
                {
                    gFreeList[freeListIndex + 1] = (int)particleIndex;
                }
                else
                {
                    InterlockedAdd(gFreeListIndex[0], -1);
                }
            }
        }
    }
}