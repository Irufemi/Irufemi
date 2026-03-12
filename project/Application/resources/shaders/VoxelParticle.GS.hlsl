#include "VoxelParticle.hlsli"

ConstantBuffer<PerView> gPerView : register(b0);

struct GSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float size : PSIZE;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

[maxvertexcount(4)]
void main(point GSInput input[1], inout TriangleStream<PSInput> stream)
{
	if (input[0].size <= 0.0f)
	{
		return;
	}

	float size = input[0].size * 0.1f; // 全体的なサイズ調整

	float4 center = mul(input[0].position, gPerView.viewProjection);

	PSInput output;
	output.color = input[0].color;

    // 左上
	output.position = center + float4(-size, size, 0.0f, 0.0f);
	output.uv = float2(0.0f, 0.0f);
	stream.Append(output);

    // 右上
	output.position = center + float4(size, size, 0.0f, 0.0f);
	output.uv = float2(1.0f, 0.0f);
	stream.Append(output);

    // 左下
	output.position = center + float4(-size, -size, 0.0f, 0.0f);
	output.uv = float2(0.0f, 1.0f);
	stream.Append(output);

    // 右下
	output.position = center + float4(size, -size, 0.0f, 0.0f);
	output.uv = float2(1.0f, 1.0f);
	stream.Append(output);

	stream.RestartStrip();
}