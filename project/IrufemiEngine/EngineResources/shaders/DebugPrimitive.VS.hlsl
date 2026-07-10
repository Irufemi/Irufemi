#include "DebugPrimitive.hlsli"
#include "VertexData.hlsli"
#include "PerFrame.hlsli"

// 各インスタンスのデータ
StructuredBuffer<InstanceData> gInstanceData : register(t1);
ConstantBuffer<PerFrameData> gPerFrame : register(b2);

// struct VertexShaderInput は VertexData.hlsli で定義

VertexShaderOutput main(VertexInput input, uint instanceID : SV_InstanceID)
{
	VertexShaderOutput output;
    
	InstanceData instanceData = gInstanceData[instanceID];

	float4 worldPos = mul(input.position, instanceData.world);
	float4 viewPos = mul(worldPos, gPerFrame.view);
	output.position = mul(viewPos, gPerFrame.projection);
	
	output.color = input.color * instanceData.color;

	return output;
}