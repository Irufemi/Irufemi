#include "ParticleGPU.hlsli"
#include "PerFrame.hlsli"

static const uint32_t kMaxParticles = 32768;

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<int32_t> gFreeList : register(u2);
ConstantBuffer<GPUParticleEmitter> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

float rand(uint seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float(seed & 0xffffff) / 16777216.0;
}

[numthreads(1, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    if (gEmitter.emit == 0) return;

    // 放出周期チェック
    if (gEmitter.frequencyTime < gEmitter.frequency) return;

    for (int32_t i = 0; i < gEmitter.count; ++i)
    {
        int32_t freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

        if (freeListIndex >= 0)
        {
            uint32_t particleIndex = gFreeList[freeListIndex];
            
            // 乱数シードの生成（フレーム時間とインデックスを組み合わせる）
            uint seed = (uint)gPerFrame.time * 1000 + i + particleIndex;
            float r1 = rand(seed);
            float r2 = rand(seed + 1337);
            float r3 = rand(seed + 7777);

            gParticles[particleIndex].currentTime = 0.0f;
            gParticles[particleIndex].lifeTime = 0.4f + r1 * 0.4f; // 0.4 ~ 0.8秒

            if (gEmitter.type == 0) // Sphere
            {
                float phi = r1 * 2.0f * 3.141592f;
                float theta = r2 * 3.141592f;
                float3 offset = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi)) * (r3 * gEmitter.radius);
                gParticles[particleIndex].translate = gEmitter.translate + offset;
                gParticles[particleIndex].velocity = normalize(offset) * 0.05f;
            }
            else if (gEmitter.type == 1) // Beam
            {
                float3 L = normalize(gEmitter.direction);
                float3 up = abs(L.y) < 0.999f ? float3(0,1,0) : float3(1,0,0);
                float3 side = normalize(cross(up, L));
                float3 upVec = cross(L, side);

                // 放出位置（円柱の断面状に分散）
                float angle = r1 * 2.0f * 3.141592f;
                float dist = sqrt(r2) * gEmitter.radius;
                float3 offset = (side * cos(angle) + upVec * sin(angle)) * dist;

                gParticles[particleIndex].translate = gEmitter.translate + offset;
                
                // 初速にバラつき（プラズマ的な噴出感）
                float3 spreadDir = (side * (r1 * 2 - 1) + upVec * (r2 * 2 - 1)) * gEmitter.spread;
                gParticles[particleIndex].velocity = (L + spreadDir) * (gEmitter.velocity * (0.8f + r3 * 0.4f));
            }

            // 配色: イエロー（コア）〜オレンジ（外装）のランダム
            float3 colorCore = float3(1.0f, 1.0f, 0.4f);
            float3 colorOuter = float3(1.0f, 0.5f, 0.1f);
            gParticles[particleIndex].color.rgb = lerp(colorOuter, colorCore, r1);
            gParticles[particleIndex].color.a = 1.0f;
            
            // 初期スケール
            float sc = 0.05f + r2 * 0.15f;
            gParticles[particleIndex].scale = float3(sc, sc, sc);
        }
        else
        {
            InterlockedAdd(gFreeListIndex[0], 1);
            break;
        }
    }
}