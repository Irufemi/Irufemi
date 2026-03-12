struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    // アルファが0以下のピクセルは描画しない
	if (input.color.a <= 0.0f)
	{
		discard;
	}
	return input.color;
}