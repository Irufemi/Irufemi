#include "Animation.hlsli"

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
StructuredBuffer<WellForGPU> gSkinCluster : register(t1); // スキニング用データ

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;

    // スキニング計算
	float32_t4 skinPos = float32_t4(0, 0, 0, 0);
	float32_t3 skinNormal = float32_t3(0, 0, 0);

    //影響を受けるボーンの数だけ計算
	for (int i = 0; i < 4; ++i)
	{
        // 影響度がある場合のみ計算
		if (input.blendWeight[i] > 0.0f)
		{
            // influence = skeletonSpaceMatrix * inverseBindPoseMatrix
			float32_t4x4 influence = mul(gSkinCluster[input.blendIndex[i]].skeletonSpaceMatrix, gSkinCluster[input.blendIndex[i]].inverseBindPoseMatrix);
			skinPos += input.blendWeight[i] * mul(input.position, influence);
			skinNormal += input.blendWeight[i] * mul(input.normal, (float32_t3x3) influence);
		}
	}
    // skinPosはローカル座標系
	skinPos.w = 1.0f; // w=1に戻す

	/*三角形を動かそう*/
	
	output.position = mul(skinPos, gTransformationMatrix.WVP);
	
	/*テクスチャを貼ろう*/
	
	///VertexShaderをtexcoord対応する
	
	output.texcoord = input.texcoord;
	
	
	/*非均一スケール*/
	
	/// 組み込んで使う
	
	// 法線を変換する際に逆転置行列を使う
	
	output.normal = normalize(mul(skinNormal, (float32_t3x3) gTransformationMatrix.WorldInverseTranspose));
	
	/*PhongReflectionModel*/
	
	/// Cameraへの方向を算出
	float32_t4 worldPos = mul(skinPos, gTransformationMatrix.World);
	output.worldPosition = worldPos.xyz;
	
	/*三角形を表示しよう*/

	return output;
}