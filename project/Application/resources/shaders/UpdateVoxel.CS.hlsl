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
			if (gParticles[particleIndex].life <= 1.0f) {
				// 速度更新（重力・上昇気流）
				if (gEmitter.particleType == 2) {
					// 燃え尽きエフェクト：上昇せず、燃えカスとして崩れ落ちる
					// 重力は受けるが、灰のように少し軽く振る舞う
					gParticles[particleIndex].velocity.y -= (gEmitter.gravity * 0.6f) * gPerFrame.deltaTime;
					// 空気抵抗（ふんわりと落ちる）
					gParticles[particleIndex].velocity *= (1.0f - 1.5f * gPerFrame.deltaTime);
					
					// 火の粉のような揺らぎ（落ちながら少し左右に舞う）
					float swayX = sin(gPerFrame.time * 15.0f + particleIndex * 0.1f) * 10.0f;
					float swayZ = cos(gPerFrame.time * 12.0f + particleIndex * 0.1f) * 10.0f;
					gParticles[particleIndex].velocity.x += swayX * gPerFrame.deltaTime;
					gParticles[particleIndex].velocity.z += swayZ * gPerFrame.deltaTime;

					// ▼ プレミアムな燃焼・炭化カラー演出
					float l = gParticles[particleIndex].life;
					if (l > 0.6f) {
						// 崩れる直前〜前半：強烈なオレンジ・赤の火の粉グラデーション（輝度加算）
						gParticles[particleIndex].color.rgb += float3(6.0f, 2.0f, 0.1f) * gPerFrame.deltaTime;
					} else {
						// 後半：急激に熱を失い、黒い炭・灰へと変わる（減算吸収）
						gParticles[particleIndex].color.rgb -= float3(8.0f, 8.0f, 8.0f) * gPerFrame.deltaTime;
						gParticles[particleIndex].color.rgb = max(float3(0.0f, 0.0f, 0.0f), gParticles[particleIndex].color.rgb);
					}
				} else if (gEmitter.particleType == 3) {
					// --- FineScatter (ヒットスパーク) ---
					// 激しく散った後、空気抵抗で急激に減速
					gParticles[particleIndex].velocity *= (1.0f - 5.0f * gPerFrame.deltaTime);
					gParticles[particleIndex].velocity.y -= gEmitter.gravity * gPerFrame.deltaTime;
					
					// 乱気流のような揺らぎを加算（飛び散ってからユラユラ舞う）
					float swayX = sin(gPerFrame.time * 20.0f + particleIndex * 0.2f) * 15.0f;
					float swayZ = cos(gPerFrame.time * 18.0f + particleIndex * 0.2f) * 15.0f;
					gParticles[particleIndex].velocity.x += swayX * gPerFrame.deltaTime;
					gParticles[particleIndex].velocity.z += swayZ * gPerFrame.deltaTime;

					// 時間経過に伴い発光が冷めて元の色・または黒い焦げ跡へ戻っていく
					float l = gParticles[particleIndex].life;
					if (l > 0.5f) {
						gParticles[particleIndex].color.rgb -= float3(5.0f, 15.0f, 20.0f) * gPerFrame.deltaTime; // 最初は火花から赤っぽく冷却
					} else {
						gParticles[particleIndex].color.rgb -= float3(20.0f, 10.0f, 5.0f) * gPerFrame.deltaTime; // 赤から黒(灰)・元の色ベースへ
						gParticles[particleIndex].color.rgb = max(float3(0.0f, 0.0f, 0.0f), gParticles[particleIndex].color.rgb);
					}
				} else {
					// 通常の重力落下
					gParticles[particleIndex].velocity.y -= gEmitter.gravity * gPerFrame.deltaTime;
				}
			
				// 収束
				float3 toCenter = gEmitter.emitPosition - gParticles[particleIndex].position;
				gParticles[particleIndex].velocity += toCenter * gEmitter.convergence * gPerFrame.deltaTime;

				// 位置更新
				gParticles[particleIndex].position += gParticles[particleIndex].velocity * gPerFrame.deltaTime;
				
				// 回転更新
				gParticles[particleIndex].rotation += gParticles[particleIndex].angularVelocity * gPerFrame.deltaTime;
				
				// 空気抵抗による回転の減衰（スピンが徐々にゆっくりになる）
				gParticles[particleIndex].angularVelocity *= (1.0f - 0.5f * gPerFrame.deltaTime);

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
			}

			// 生存時間更新
			gParticles[particleIndex].life -= (1.0f / gEmitter.lifeTime) * gPerFrame.deltaTime;
        
			// 色(アルファ)とサイズ更新
			gParticles[particleIndex].color.a = saturate(gParticles[particleIndex].life);
			if (gEmitter.particleType == 2) {
				// 燃えカスは早めに小さくなりながら消える
				gParticles[particleIndex].size = saturate(gParticles[particleIndex].life * 1.5f);
			} else if (gEmitter.particleType == 3) {
				// ヒットスパーク（初期サイズを少し大きくし、点滅するように素早く小さくする）
				gParticles[particleIndex].size = 0.5f * saturate(gParticles[particleIndex].life * 4.0f);
			} else {
				gParticles[particleIndex].size = saturate(gParticles[particleIndex].life * 5.0f); // 最後の20%で縮小
			}
		} else {
			// 寿命が尽きたら非アクティブにする
			gParticles[particleIndex].isActive = 0;
			gParticles[particleIndex].life = 0.0f;
			gParticles[particleIndex].color.a = 0.0f;
		}
	}
}