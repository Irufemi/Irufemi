#include "Line.hlsli"

// material を PS 側で参照(register を b0 に合わせる)
ConstantBuffer<Material> gMaterial : register(b0);

#include "Bindless.hlsli"

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
	float32_t4 mask  : SV_TARGET1;
	float32_t4 normal : SV_TARGET2;
	float32_t4 material : SV_TARGET3;
	float32_t2 velocity : SV_TARGET4;
};

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	
	// PS 側で material を使って最終色を決定
	output.color = gMaterial.color * input.color;
	output.mask = float32_t4(0.0f, 0.0f, 1.0f, 1.0f);
	
	output.normal = float32_t4(0.0f, 0.0f, 0.0f, 1.0f);
	output.material = float32_t4(0.0f, 0.0f, 0.0f, 1.0f);
	output.velocity = float32_t2(0.0f, 0.0f);
	
	return output;
}