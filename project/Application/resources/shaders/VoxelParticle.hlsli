#ifndef VOXEL_PARTICLE_HLSLI
#define VOXEL_PARTICLE_HLSLI

// CPU側のVoxel構造体と対応
struct Voxel
{
	float3 position; // 初期位置 (AABB Minからのオフセット)
	float4 color;
	float3 normal;
};

// GPU上で更新されるパーティクルごとのデータ
struct VoxelParticle
{
	float3 position;
	float3 velocity;
	float4 color;
	float life;
	float size;
	uint isActive; // 0:非アクティブ, 1:アクティブ
};

// エフェクト全体を制御する定数バッファ
struct VoxelEmitter
{
	float3 emitPosition; // エフェクトの発生基点
	float time; // エフェクト開始からの経過時間
	float lifeTime; // パーティクルの最大寿命
	float gravity; // 重力加速度
	uint emit; // 射出トリガー (0以外で射出)
	float dispersion; // 爆発の散開係数
	float convergence; // 収束係数
};

struct PerView
{
	float4x4 viewProjection;
	float4x4 billboard;
};


#endif // VOXEL_PARTICLE_HLSLI