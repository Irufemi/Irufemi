struct Particle
{
	float3 translate;
	float3 scale;
	float lifeTime;
	float3 velocity;
	float currentTime;
	float4 color;

	// 拡張パラメータ
	float3 rotation;
	float3 rotateSpeed;
	float3 startScale;
	float3 endScale;
	float4 startColor;
	float4 endColor;
};

struct PerView
{
	float4x4 viewProjection;
	float4x4 billboardMatrix;
};

struct VertexShaderOutput
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct GPUParticleEmitter
{
	// float4 x 1
	uint type;          // 0: Sphere, 1: Beam, 2: Ring, 3: Cylinder
	float3 translate;   // 位置

	// float4 x 2
	int count;          // 放出数
	float frequency;    // 頻度
	float frequencyTime;// タイマー
	int emit;           // 放出フラグ

	// float4 x 3
	float radius;       // Sphere/Ring/Cylinder用: 半径
	float3 direction;   // Beam用: 方向

	// float4 x 4
	float spread;       // Beam用: 広がり
	float velocity;     // Beam用: 速度
	float minLife;      // 最小寿命
	float maxLife;      // 最大寿命

	// float4 x 5
	float3 startScaleMin; // 開始スケール最小
	float pad0;
	// float4 x 6
	float3 startScaleMax; // 開始スケール最大
	float pad1;
	// float4 x 7
	float3 endScaleMin;   // 終了スケール最小
	float pad2;
	// float4 x 8
	float3 endScaleMax;   // 終了スケール最大
	float pad3;

	// float4 x 9
	float4 startColorMin; // 開始色最小
	// float4 x 10
	float4 startColorMax; // 開始色最大
	// float4 x 11
	float4 endColorMin;   // 終了色最小
	// float4 x 12
	float4 endColorMax;   // 終了色最大

	// float4 x 13
	uint colorMode;       // カラーモード
	float gravity;        // 重力
	float damping;        // 空気抵抗
	uint isBillboard;     // ビルボードフラグ

	// float4 x 14
	uint burstCount;      // そのフレームの追加放出数
	float jitter;         // 座標のゆらぎ
	uint atlasRows;
	uint atlasCols;

	// float4 x 15
	float groundHeight;
	float bounce;
	float attractorStrength;
	uint pad4;

	// float4 x 16
	float3 attractorPos;
	uint pad5;

	// float4 x 17
	float3 areaSize;
	uint pad6;
};