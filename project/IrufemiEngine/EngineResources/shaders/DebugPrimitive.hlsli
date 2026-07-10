// インスタンスごとのデータ
struct InstanceData
{
	float32_t4x4 world;
	float32_t4 color;
};

struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t4 color : COLOR0;
};