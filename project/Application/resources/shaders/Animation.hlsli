struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	float32_t2 texcoord : TEXCOORD0;
	float32_t3 normal : NORMAL0;
	int4 blendIndex : BLENDINDICES0; // ボーンインデックス
	float4 blendWeight : BLENDWEIGHT0; // ボーンウェイト
};

struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t2 texcoord : TEXCOORD0;
	float32_t3 normal : NORMAL0;
	float32_t3 worldPosition : POSITION0;
};

struct TransformationMatrix
{
	float32_t4x4 WVP;
	float32_t4x4 World;
	float32_t4x4 WorldInverseTranspose;
};

// スキニング用の行列を格納する構造体
struct WellForGPU
{
	float32_t4x4 skeletonSpaceMatrix;
	float32_t4x4 inverseBindPoseMatrix;
};