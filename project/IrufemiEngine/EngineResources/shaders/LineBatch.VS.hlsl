#include "LineBatch.hlsli"
#include "VertexData.hlsli"
#include "PerFrame.hlsli"

ConstantBuffer<PerFrameData> gPerFrame : register(b2);
StructuredBuffer<InstanceData> gInstanceData : register(t1);

VertexShaderOutput main(VertexInput input, uint instanceID : SV_InstanceID)
{
	VertexShaderOutput output;
    
	InstanceData instanceData = gInstanceData[instanceID];

	// input.position.x = 0.0 -> start, 1.0 -> end
	float3 pos = lerp(instanceData.start.xyz, instanceData.end.xyz, input.position.x);

	float4 viewPos = mul(float4(pos, 1.0f), gPerFrame.view);
	output.position = mul(viewPos, gPerFrame.projection);
	output.color = input.color * instanceData.color;

	return output;
}
