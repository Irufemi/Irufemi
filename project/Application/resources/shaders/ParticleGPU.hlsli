
struct Particle
{
	float32_t3 translate;
	float32_t3 scale;
	float32_t lifeTime;
	float32_t3 velocity;
	float32_t currentTime;
	float32_t4 color;
};

struct PerView
{
	float32_t4x4 viewProjection;
	float32_t4x4 billbordMatrix;
};

struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t2 texcoord : TEXCOORD0;
	float32_t4 color : COLOR0;
};

struct EmitterSphere
{
	float32_t3 translate;
	float32_t radius;
	int32_t count;
	float32_t frequency;
	float32_t frequencyTime;
	int32_t emit;
};