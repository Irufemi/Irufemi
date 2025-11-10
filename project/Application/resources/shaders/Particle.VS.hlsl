/*テクスチャを貼ろう*/

#include "./Particle.hlsli"

/*三角形を動かそう*/

struct ParticleForGPU
{
	float32_t4x4 WVP;
	
	/*LambertianReflectance*/
	
	float32_t4x4 World;
	
	float32_t4 color;
};
StructuredBuffer<ParticleForGPU> gParticle : register(t0);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
	
	/*テクスチャを貼ろう*/
	
	///VertexShaderをtexcoord対応する
	
	float32_t2 texcoord : TEXCOORD0;
	
    /*LambertianReflectance*/
	
	float32_t3 normal : NORMAL0;
	
};

/*テクスチャを貼ろう*/

VertexShaderOutput main(VertexShaderInput input, uint32_t instanced : SV_InstanceID)
{
	VertexShaderOutput output;
	//output.position = input.position;
	
	/*三角形を動かそう*/
	
	output.position = mul(input.position, gParticle[instanced].WVP);
	
	/*テクスチャを貼ろう*/
	
	///VertexShaderをtexcoord対応する
	
	output.texcoord = input.texcoord;
	
	
	/*LambertianReflectance*/
	
	///法線の座標系を変換してPixelShaderに送る
	
	output.color = gParticle[instanced].color;
	
	/*三角形を表示しよう*/

	return output;
}

