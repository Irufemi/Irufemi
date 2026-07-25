#include "LineBatch.hlsli"

struct PixelShaderOutput
{
	float32_t4 color : SV_TARGET0;
	float32_t4 mask  : SV_TARGET1;
};

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	output.color = input.color;
	output.mask = float32_t4(0.0f, 0.0f, 1.0f, 1.0f);
	return output;
}
