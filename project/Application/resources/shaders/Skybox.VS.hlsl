#include "skybox.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
	float32_t4 position : POSITION0;
};


struct Camera {
	float32_t4x4 view;
	float32_t4x4 projection;
	float32_t3 worldPosition;
};
ConstantBuffer<Camera> gCamera : register(b2);

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	
	float32_t4 worldPos = mul(input.position, gTransformationMatrix.World);
	// スカイボックスはカメラの位置に追従するため、ビュー行列の平行移動成分を無効化する
	float4x4 viewNoTranslation = gCamera.view;
	viewNoTranslation[3][0] = 0.0f;
	viewNoTranslation[3][1] = 0.0f;
	viewNoTranslation[3][2] = 0.0f;
	
	float4 viewPos = mul(worldPos, viewNoTranslation);
	output.position = mul(viewPos, gCamera.projection).xyww;
	
	output.texcoord = input.position.xyz;

	return output;
}

