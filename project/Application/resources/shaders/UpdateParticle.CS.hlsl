#include "ParticleGPU.hlsli"
#include "PerFrame.hlsli"

static const uint32_t kMaxParticles = 32768;

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int32_t> gFreeListIndex : register(u1);
RWStructuredBuffer<int32_t> gFreeList : register(u2);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

// 簡易的なハッシュ関数
float rand(uint seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return float(seed & 0xffffff) / 16777216.0;
}

[numthreads(1024, 1, 1)]
void main(uint32_t3 DTid : SV_DispatchThreadID)
{
    uint32_t particleIndex = DTid.x;
    if (particleIndex < kMaxParticles)
    {
        if (gParticles[particleIndex].color.a > 0.0f)
        {
            // 進捗 (0.0: 生まれたて, 1.0: 寿命)
            float t = gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime;
            
            // 1. 移動更新（わずかなゆらぎ(Jitter)を追加）
            float rndX = rand(particleIndex + (uint)gPerFrame.time * 100) * 2.0 - 1.0;
            float rndY = rand(particleIndex + (uint)gPerFrame.time * 200 + 1) * 2.0 - 1.0;
            float3 jitter = float3(rndX, rndY, 0.0) * 0.01f; // 進行方向に垂直な面での微振動感覚
            
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity + jitter;
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;

            // 2. カラー更新: 高温（黄）-> 低温（赤）への遷移
            float3 brightColor = float3(1.0f, 1.0f, 0.3f); // コアカラー
            float3 darkColor   = float3(1.0f, 0.1f, 0.0f); // 外装カラー
            gParticles[particleIndex].color.rgb = lerp(brightColor, darkColor, saturate(t * 1.5f));
            
            float alpha = 1.0f - saturate(t);
            gParticles[particleIndex].color.a = alpha;

            // 3. スケール更新: ライフサイクルに合わせて少し膨張してから消滅
            // 0.5付近で最大、後半で急激に縮小
            float scaleFactor = sin(saturate(t) * 3.141592f);
            // 元のスケール（Emit時に設定されたもの）をベースにするのが理想だが
            // ここでは簡易的に 1.5倍まで膨らませる
            if (t > 0.0f) {
                // translate等と同様に毎フレーム上書き or 係数掛け
                // EmitParticle側で初期スケールを大きめに設定し、ここでは徐々に小さくする
                gParticles[particleIndex].scale *= (1.0f - gPerFrame.deltaTime * 2.0f);
            }

            // 寿命終了判定
            if (gParticles[particleIndex].color.a <= 0.0f)
            {
                gParticles[particleIndex].scale = float32_t3(0.0f, 0.0f, 0.0f);
                int32_t freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                if ((freeListIndex + 1) < (int32_t)kMaxParticles)
                {
                    gFreeList[freeListIndex + 1] = particleIndex;
                }
                else
                {
                    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
                }
            }
        }
    }
}