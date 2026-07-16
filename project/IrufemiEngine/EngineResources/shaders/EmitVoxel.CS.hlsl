#include "VoxelParticle.hlsli"
#include "RandomGenerator.hlsli"
#include "PerFrame.hlsli"

StructuredBuffer<Voxel> gVoxels : register(t0);
StructuredBuffer<VoxelEmitter> gEmitters : register(t1);

RWStructuredBuffer<VoxelParticle> gParticles : register(u0);

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
    uint localVoxelIndex = particleIndex % gVoxelCount;

	Voxel voxel = gVoxels[localVoxelIndex];
    VoxelEmitter emitter = gEmitters[emitterIndex];
    
    if (emitter.emit == 0) {
        return;
    }

    // 既にアクティブで寿命が残っている場合は再配置しない（連続エミット時のリセット防止）
    if (gParticles[particleIndex].isActive == 1 && gParticles[particleIndex].life > 0.0f) {
        return;
    }

	RandomGenerator generator;
    // 乱数のシードを時間とインデックスで初期化
	generator.seed = particleIndex * 12345 + uint3(1, 7, 11) * (uint) (gPerFrame.time * 1000.0f);

    // 1. スケーリング
	float3 localPos = voxel.position * emitter.scale;
    
    // 2. 回転 (XYZの順で回転行列を作成して適用)
	float cX = cos(emitter.rotate.x);
	float sX = sin(emitter.rotate.x);
	float cY = cos(emitter.rotate.y);
	float sY = sin(emitter.rotate.y);
	float cZ = cos(emitter.rotate.z);
	float sZ = sin(emitter.rotate.z);

	float3x3 rotX = { 1, 0, 0, 0, cX, -sX, 0, sX, cX };
	float3x3 rotY = { cY, 0, sY, 0, 1, 0, -sY, 0, cY };
	float3x3 rotZ = { cZ, -sZ, 0, sZ, cZ, 0, 0, 0, 1 };
	float3x3 rotateMat = mul(rotZ, mul(rotY, rotX));
    
	localPos = mul(rotateMat, localPos);
    
    // 3. 初期位置: エミッター位置 + 回転・スケール適用後のボクセル相対位置
	float3 worldPos = emitter.emitPosition + localPos;
	gParticles[particleIndex].position = worldPos;

	// OBB判定 (衝突領域内かチェック)
	if (emitter.useCollision != 0)
	{
		float3 d = worldPos - emitter.collisionCenter;
		bool inside = true;
		[unroll]
		for (int i = 0; i < 3; ++i)
		{
			float dist = dot(d, emitter.collisionOrientations[i].xyz);
			if (abs(dist) > emitter.collisionSize[i])
			{
				inside = false;
				break;
			}
		}

		if (!inside)
		{
			// 既に飛散中の他のボクセルを消さないように、ここでは単にスキップするだけにする
			return;
		}
	}
    
    // 4. 初期速度: ボクセルの法線を回転させ、衝突時は中心から外側へ向かうベクトルを加味する
	float3 rotatedNormal = normalize(mul(rotateMat, voxel.normal));
	float3 burstDir = (emitter.useCollision != 0) ? normalize(worldPos - emitter.collisionCenter) : rotatedNormal;
	
	float3 randomVec = (generator.Generate3d() * 2.0f - 1.0f) * 0.5f; // -0.5 ~ 0.5
    float3 finalVelocity = float3(0,0,0);

    if (emitter.particleType == 1 && emitter.useCollision != 0)
    {
        // ビル近接攻撃などの衝突飛散時：均一に吹き飛ぶ不自然さを解消
        float speedVariance = generator.Generate1d() * 0.6f + 0.4f; // 0.4 ~ 1.0 の速度ブレ
        
        float hitSpeed = length(emitter.baseVelocity);
        float3 hitDir = (hitSpeed > 0.001f) ? normalize(emitter.baseVelocity) : float3(0, 1, 0);
        
        // 打撃の進行方向(hitDir) と 衝突中心から外への方向(burstDir) をブレンドし、ランダム成分を足す
        float3 scatterDir = normalize(lerp(burstDir, hitDir, 0.4f) + randomVec);
        
        // 上に向かって破片が散るように Y 成分を少し底上げ
        scatterDir.y += abs(randomVec.y) * 0.8f + 0.2f;
        scatterDir = normalize(scatterDir);
        
        // 打撃の強さ(hitSpeed)を一部利用しつつ、バラつき(speedVariance)を与えることで塊で飛ぶのを防ぐ
        finalVelocity = scatterDir * (hitSpeed * 0.5f + emitter.dispersion) * speedVariance;
    }
    else
    {
        // 既存ロジック
	    float3 moveDir = normalize(lerp(rotatedNormal, burstDir, 0.7f) + randomVec);
	    finalVelocity = emitter.baseVelocity + moveDir * emitter.dispersion;
    }

	gParticles[particleIndex].color = voxel.color;
	
	float delay = 0.0f;
	if (emitter.particleType == 2) // AshDisintegration
	{
		// 下から崩れるように、ローカルのY座標に応じたディレイを計算
		float noise = (generator.Generate1d() * 2.0f - 1.0f) * 0.2f;
		delay = max(0.0f, localPos.y * 0.05f + noise);
	}
	else if (emitter.particleType == 4) { // DebrisLargeGravity
		// エネミー破壊：真下に重く落ちる（横への広がりを抑え、下向きベクトルを付与）
		float3 randVec = generator.Generate3d() * 2.0f - 1.0f;
		finalVelocity = float3(randVec.x * 0.2f, -1.0f - randVec.y, randVec.z * 0.2f) * length(emitter.baseVelocity);
	}
	else if (emitter.particleType == 5) { // DebrisExplosive
		// プレイヤー寿命：空中で放射状に鋭く四散しつつ、元の吹き飛び方向（baseVelocity）の勢いも引き継ぐ
		float3 randDir = normalize(generator.Generate3d() * 2.0f - 1.0f);
		// baseVelocity（吹き飛びの速度）をベースにしつつ、全方向への散らばりを加える
		finalVelocity = emitter.baseVelocity * 0.7f + randDir * (length(emitter.baseVelocity) * 0.3f + emitter.dispersion * 3.0f);
	}
	else if (emitter.particleType == 3) // FineScatter (被弾時)
	{
		if (dot(finalVelocity, rotatedNormal) < 0.0f)
		{
			finalVelocity = reflect(finalVelocity, rotatedNormal);
			finalVelocity += rotatedNormal * (emitter.dispersion * 0.5f);
		}

		// パラメータ駆動の色変更
		float hitNoise = generator.Generate1d();
		float3 sparkColor = lerp(emitter.endColor.rgb, emitter.startColor.rgb, hitNoise);
		gParticles[particleIndex].color.rgb = voxel.color.rgb * sparkColor;
	}
	
	gParticles[particleIndex].velocity = finalVelocity;
	gParticles[particleIndex].life = 1.0f + delay; // 寿命を満タンにする＋ディレイを加算
	gParticles[particleIndex].size = 1.0f; // サイズ
	gParticles[particleIndex].isActive = 1; // アクティブ化
	gParticles[particleIndex].normal = rotatedNormal; // 回転後の法線をコピー

    // 5. 初期回転と角速度の付与
    gParticles[particleIndex].rotation = emitter.rotate; 
    
    // 角速度（スピン）をランダムに設定
    float3 randomSpin = (generator.Generate3d() * 2.0f - 1.0f) * emitter.spinSpeed;
    if (emitter.particleType == 3) { 
        randomSpin *= 1.5f;
    }
    gParticles[particleIndex].angularVelocity = randomSpin;
}