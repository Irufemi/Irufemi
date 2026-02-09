
/*テクスチャを貼ろう*/

///Object3d/hlsliを使うようにする

struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t3 texcoord : TEXCOORD0;
};

struct TransformationMatrix
{
	float32_t4x4 WVP;
};