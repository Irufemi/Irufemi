#include "Transform.hlsli"
#include "BasePassVertexOutput.hlsli"
#include "Lighting.hlsli"
#include "VertexData.hlsli"

ConstantBuffer<LightCommonData> gLightCommonData : register(b1);

struct InstanceData
{
	float32_t4x4 WVP;
	float32_t4x4 World;
	float32_t4x4 WorldInverseTranspose;
	float32_t4 color;
};
StructuredBuffer<InstanceData> gBlocks : register(t0);

VertexShaderOutput main(VertexInput input, uint32_t instanceId : SV_InstanceID)
{
	VertexShaderOutput output;

	InstanceData inst = gBlocks[instanceId];

    // ワールド座標
	float32_t4 worldPos = mul(input.position, inst.World);

	// ライト空間座標への変換 (デプスのみ必要だが整合性のため出力する)
	output.position = mul(worldPos, gLightCommonData.viewProjection);

	output.texcoord = input.texcoord;
	
    float32_t4 n4 = mul(float32_t4(input.normal, 0.0f), inst.WorldInverseTranspose);
	output.normal = normalize(n4.xyz);
	
    output.worldPosition = worldPos.xyz;
	output.shadowPos = output.position;
	output.color = input.color * inst.color;

	return output;
}
