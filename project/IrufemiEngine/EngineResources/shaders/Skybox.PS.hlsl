#include "GBufferOutput.hlsli"
#include "skyBox.hlsli"

struct Material
{
	float32_t4 color;
	float32_t intensity;
	uint32_t textureIndex;
	uint32_t padding[2];
};
ConstantBuffer<Material> gMaterial : register(b0);

#include "Bindless.hlsli"



SamplerState gSampler : register(s0); //Samplerのregisterはs

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	float32_t4 textureColor = gTextureCubes[gMaterial.textureIndex].Sample(gSampler, input.texcoord);
	
	output.color = textureColor * gMaterial.color * gMaterial.intensity * input.color;
	output.mask = float32_t4(0.0f, 0.0f, 0.0f, 0.0f); // 背景はマスクされない
	output.normal = float32_t4(0.0f, 0.0f, 0.0f, 0.0f); // 背景なので適当
	output.material = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
	output.velocity = float32_t2(0.0f, 0.0f);
	
	return output;
}