
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
	float32_t4x4 billboardMatrix;
};

struct VertexShaderOutput
{
	float32_t4 position : SV_POSITION;
	float32_t2 texcoord : TEXCOORD0;
	float32_t4 color : COLOR0;
};

struct GPUParticleEmitter
{
	uint32_t type;          // 0: Sphere, 1: Beam
	float32_t3 translate;   // 位置

	int32_t count;          // 放出数
	float32_t frequency;    // 頻度
	float32_t frequencyTime;// タイマー
	int32_t emit;           // 放出フラグ

	float32_t radius;       // Sphere用: 半径
	float32_t3 direction;   // Beam用: 方向

	float32_t spread;       // Beam用: 広がり（0: 直線, 1: 全方位）
	float32_t velocity;     // Beam用: 速度
	float32_t2 pad;
};