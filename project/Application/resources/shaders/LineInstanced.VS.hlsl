#include "LineInstanced.hlsli"

// 各インスタンスのデータ
StructuredBuffer<InstanceData> gInstanceData : register(t1);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	uint instanceID : SV_InstanceID;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
    
	InstanceData instanceData = gInstanceData[input.instanceID];

	output.position = mul(input.position, instanceData.WVP);
	output.color = instanceData.color;

	return output;
}