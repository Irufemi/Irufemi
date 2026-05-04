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
    // 放出数の計算（通常放出 + バースト放出）
    int emitCount = (int)gEmitter.burstCount;

    if (emitCount <= 0) return;

    int i = (int)DTid.x;
    if (i >= emitCount) return;

    int freeListIndex;
    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

        if (freeListIndex >= 0 && freeListIndex < (int)kMaxParticles)
        {
            uint particleIndex = (uint)gFreeList[freeListIndex];
            
            // 乱数生成器の初期化
            // iとparticleIndexが連動して相殺するのを防ぐため素数を掛ける。さらにエミッターごとのシードを加味して完全一致を防ぐ。
            uint seedValue = (uint)gPerFrame.time * 100000 + (i * 1337) + particleIndex + (gEmitter.randomSeed * 77777);
            RandomGenerator rng;
            rng.seed = uint3(seedValue, seedValue + 111, seedValue + 222);

            float r_life  = rng.Generate1d();
            float r_scale = rng.Generate1d();
            float r_color = rng.Generate1d();
            float3 r_pos  = rng.Generate3d();
            float r_vel   = rng.Generate1d();

            gParticles[particleIndex].currentTime = 0.0f;
            gParticles[particleIndex].lifeTime = max(lerp(gEmitter.minLife, gEmitter.maxLife, r_life), 0.0001f);

            // 放出形状別の初期位置・速度設定
            if (gEmitter.type == 0) // Sphere
            {
                float phi = r_pos.x * 2.0f * 3.141592f;
                float theta = r_pos.y * 3.141592f;
                float3 offset = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi)) * (r_pos.z * gEmitter.radius);
                gParticles[particleIndex].translate = gEmitter.translate + offset;
                float3 radialDir = normalize(offset + float3(0.0001f, 0.0001f, 0.0001f));
                gParticles[particleIndex].velocity = (gEmitter.direction + radialDir * gEmitter.spread) * gEmitter.velocity;
            }
            else if (gEmitter.type == 1) // Beam
            {
                float3 L = normalize(gEmitter.direction);
                float3 up = abs(L.y) < 0.999f ? float3(0,1,0) : float3(1,0,0);
                float3 side = normalize(cross(up, L));
                float3 upVec = cross(L, side);

                float angle = r_pos.x * 2.0f * 3.141592f;
                float dist = sqrt(r_pos.y) * gEmitter.radius;
                float3 offset = (side * cos(angle) + upVec * sin(angle)) * dist;

                gParticles[particleIndex].translate = gEmitter.translate + offset;
                float3 spreadDir = (side * (r_pos.x * 2 - 1) + upVec * (r_pos.y * 2 - 1)) * gEmitter.spread;
                gParticles[particleIndex].velocity = (L + spreadDir) * (gEmitter.velocity * (0.8f + r_vel * 0.4f));
            }
            else if (gEmitter.type == 2) // Ring
            {
                float angle = rng.Generate1d() * 2.0f * 3.141592f;
                // radius: 外径, 厚みは既存の計算でspreadを流用していたが、放射強度のspreadと被るのでここでは固定値の0.1などに固定するか、そのまま使う
                float r = gEmitter.radius - (rng.Generate1d() * 0.1f);
                float3 offset = float3(cos(angle), 0, sin(angle)) * r;
                
                gParticles[particleIndex].translate = gEmitter.translate + offset;
                float3 radialDir = normalize(offset + float3(0.0001f, 0.0001f, 0.0001f));
                gParticles[particleIndex].velocity = (gEmitter.direction + radialDir * gEmitter.spread) * gEmitter.velocity;
            }
            else if (gEmitter.type == 3) // Cylinder
            {
                float angle = rng.Generate1d() * 2.0f * 3.141592f;
                float r = rng.Generate1d() * gEmitter.radius;
                float h = (rng.Generate1d() * 2.0f - 1.0f) * (gEmitter.velocity * 0.5f); // velocityを高さとして流用
                
                float3 L = normalize(gEmitter.direction);
                float3 up = abs(L.y) < 0.999f ? float3(0,1,0) : float3(1,0,0);
                float3 side = normalize(cross(up, L));
                float3 upVec = cross(L, side);
                
                float3 offset = (side * cos(angle) + upVec * sin(angle)) * r + L * h;
                gParticles[particleIndex].translate = gEmitter.translate + offset;
                gParticles[particleIndex].velocity = L * 0.05f;
            }
            else if (gEmitter.type == 4) // Box
            {
                float3 offset = (r_pos - float3(0.5f, 0.5f, 0.5f)) * gEmitter.areaSize;
                gParticles[particleIndex].translate = gEmitter.translate + offset;
                float3 radialDir = normalize(offset + float3(0.0001f, 0.0001f, 0.0001f));
                gParticles[particleIndex].velocity = (gEmitter.direction + radialDir * gEmitter.spread) * gEmitter.velocity;
            }

            // スケール初期化
            gParticles[particleIndex].startScale = lerp(gEmitter.startScaleMin, gEmitter.startScaleMax, r_scale);
            gParticles[particleIndex].endScale = lerp(gEmitter.endScaleMin, gEmitter.endScaleMax, r_scale);
            gParticles[particleIndex].scale = gParticles[particleIndex].startScale;

            // カラー初期化
            gParticles[particleIndex].startColor = lerp(gEmitter.startColorMin, gEmitter.startColorMax, r_color);
            gParticles[particleIndex].endColor = lerp(gEmitter.endColorMin, gEmitter.endColorMax, r_color);
            gParticles[particleIndex].color = gParticles[particleIndex].startColor;

            // 回転初期化
            if (gEmitter.enableRandomRotation != 0) {
                gParticles[particleIndex].rotation = rng.Generate3d() * 2.0f * 3.141592f;
                gParticles[particleIndex].rotateSpeed = (rng.Generate3d() * 2.0f - 1.0f) * 3.141592f;
            } else {
                gParticles[particleIndex].rotation = float3(0.0f, 0.0f, 0.0f);
                gParticles[particleIndex].rotateSpeed = float3(0.0f, 0.0f, 0.0f);
            }
        }
        else
    {
        InterlockedAdd(gFreeListIndex[0], 1);
    }
}