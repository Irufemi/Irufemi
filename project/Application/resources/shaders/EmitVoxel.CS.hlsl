#include "VoxelParticle.hlsli"
#include "RandomGenerator.hlsli"
#include "PerFrame.hlsli"

StructuredBuffer<Voxel> gVoxels : register(t0);
RWStructuredBuffer<VoxelParticle> gParticles : register(u0);
ConstantBuffer<VoxelEmitter> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	uint voxelIndex = dispatchThreadID.x;

	// バッファ範囲チェック
	uint count, stride;
	gVoxels.GetDimensions(count, stride);
	if (voxelIndex >= count)
		return;

	Voxel voxel = gVoxels[voxelIndex];
    
	RandomGenerator generator;
    // 乱数のシードを時間とインデックスで初期化
	generator.seed = voxelIndex * 12345 + uint3(1, 7, 11) * (uint) (gPerFrame.time * 1000.0f);

    // 1. スケーリング
	float3 localPos = voxel.position * gEmitter.scale;
    
    // 2. 回転 (XYZの順で回転行列を作成して適用)
	float cX = cos(gEmitter.rotate.x);
	float sX = sin(gEmitter.rotate.x);
	float cY = cos(gEmitter.rotate.y);
	float sY = sin(gEmitter.rotate.y);
	float cZ = cos(gEmitter.rotate.z);
	float sZ = sin(gEmitter.rotate.z);

	float3x3 rotX = { 1, 0, 0, 0, cX, -sX, 0, sX, cX };
	float3x3 rotY = { cY, 0, sY, 0, 1, 0, -sY, 0, cY };
	float3x3 rotZ = { cZ, -sZ, 0, sZ, cZ, 0, 0, 0, 1 };
	float3x3 rotateMat = mul(rotZ, mul(rotY, rotX));
    
	localPos = mul(rotateMat, localPos);
    
    // 3. 初期位置: エミッター位置 + 回転・スケール適用後のボクセル相対位置
	float3 worldPos = gEmitter.emitPosition + localPos;
	gParticles[voxelIndex].position = worldPos;

	// OBB判定 (衝突領域内かチェック)
	if (gEmitter.useCollision != 0)
	{
		float3 d = worldPos - gEmitter.collisionCenter;
		bool inside = true;
		[unroll]
		for (int i = 0; i < 3; ++i)
		{
			float dist = dot(d, gEmitter.collisionOrientations[i].xyz);
			if (abs(dist) > gEmitter.collisionSize[i])
			{
				inside = false;
				break;
			}
		}

		if (!inside)
		{
			gParticles[voxelIndex].isActive = 0;
			return;
		}
	}
    
    // 4. 初期速度: ボクセルの法線を回転させ、衝突時は中心から外側へ向かうベクトルを加味する
	float3 rotatedNormal = normalize(mul(rotateMat, voxel.normal));
	float3 burstDir = (gEmitter.useCollision != 0) ? normalize(worldPos - gEmitter.collisionCenter) : rotatedNormal;
	
	float3 randomVec = (generator.Generate3d() * 2.0f - 1.0f) * 0.5f; // -0.5 ~ 0.5
	float3 moveDir = normalize(lerp(rotatedNormal, burstDir, 0.7f) + randomVec);
	
	gParticles[voxelIndex].velocity = gEmitter.baseVelocity + moveDir * gEmitter.dispersion;

	gParticles[voxelIndex].color = voxel.color;
	gParticles[voxelIndex].life = 1.0f; // 寿命を満タンにする
	gParticles[voxelIndex].size = 1.0f; // サイズ
	gParticles[voxelIndex].isActive = 1; // アクティブ化
	gParticles[voxelIndex].normal = rotatedNormal; // 回転後の法線をコピー
}