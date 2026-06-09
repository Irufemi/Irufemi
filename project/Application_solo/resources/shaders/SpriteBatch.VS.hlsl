// SpriteBatch.VS.hlsl
// Instancing VS for 2D Sprites

#include "./Object2D.hlsli"
#include "VertexData.hlsli"

struct InstanceData
{
	float32_t4x4 WVP;
	float32_t4 color;
};

// t0にInstanceDataの配列をバインド
StructuredBuffer<InstanceData> gInstances : register(t0);

// VertexInputはVertexData.hlsliで定義されている
VertexShaderOutput main(VertexInput input, uint32_t instanceId : SV_InstanceID)
{
	VertexShaderOutput output;

	InstanceData inst = gInstances[instanceId];

	// WVP だけで投影（2D用なのでWorld行列などは不要）
	output.position = mul(input.position, inst.WVP);

	// UV はそのまま
	output.texcoord = input.texcoord;

	// Unlit 想定なので法線は固定(PSでは未使用)
	output.normal = float32_t3(0.0f, 0.0f, -1.0f);
	
	output.color = input.color * inst.color;

	return output;
}
