#include "VoxelParticle.hlsli"

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float size : PSIZE;
};

StructuredBuffer<VoxelParticle> gParticles : register(t0);

VSOutput main(uint vertexID : SV_VertexID)
{
	VSOutput output;
	VoxelParticle particle = gParticles[vertexID];

    // アクティブなパーティクルのみ描画
	if (particle.isActive == 0 || particle.life <= 0.0f)
	{
		output.position = float4(0.0, 0.0, 0.0, 0.0); // 画面外に飛ばす
		output.color = float4(0.0, 0.0, 0.0, 0.0);
		output.size = 0.0;
	}
	else
	{
		output.position = float4(particle.position, 1.0f);
		output.color = particle.color;
		output.size = particle.size;
	}

	return output;
}