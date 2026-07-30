
/*テクスチャを貼ろう*/

///Object3d/hlsliを使うようにする

#include "BasePassVertexOutput.hlsli"
#include "Transform.hlsli"

struct Well
{
	float32_t4x4 skeletonSpaceMatrix;
	float32_t4x4 skeletonInverseTransposeMatrix;
};