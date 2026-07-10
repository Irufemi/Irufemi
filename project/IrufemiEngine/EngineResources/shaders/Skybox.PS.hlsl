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

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
};

SamplerState gSampler : register(s0); //Samplerのregisterはs

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	float32_t4 textureColor = gTextureCubes[gMaterial.textureIndex].Sample(gSampler, input.texcoord);
	
	output.color = textureColor * gMaterial.color * gMaterial.intensity * input.color;
	
	return output;
}