#include "LineBatch.hlsli"

#include "BasePassPixelOutput.hlsli"

PixelShaderOutput main(VertexShaderOutput input)
{
	PixelShaderOutput output;
	output.color = input.color;
	output.mask = float32_t4(0.0f, 0.0f, 1.0f, 1.0f);
	output.normal = float32_t4(0.0f, 0.0f, 0.0f, 1.0f);
	output.material = float32_t4(0.0f, 0.0f, 0.0f, 1.0f);
	output.velocity = float32_t2(0.0f, 0.0f);
	return output;
}
