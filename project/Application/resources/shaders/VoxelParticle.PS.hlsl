#include "VoxelParticle.hlsli"

struct DirectionalLight
{
	float4 color;
	float3 direction;
	float intensity;
};

struct Camera
{
	float4x4 view;
	float4x4 projection;
	float3 worldPosition;
};

// 定数バッファはバインドエラーを避けるため使用せず、シェーダー内で完結させる

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
	// アルファが0以下のピクセルは描画しない（爆発後、生存時間が切れたパーティクル）
	if (input.color.a <= 0.0f)
	{
		discard;
	}

	// ライティング計算
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 specular = float3(0.0f, 0.0f, 0.0f);

	// Voxelの基本色 (テクスチャサンプリングされた色、またはデフォルト色)
	float3 baseColor = input.color.rgb;

	// 固定のライト設定
	float3 lightDir = normalize(float3(1.0f, -1.0f, 1.0f));
	float3 lightColor = float3(1.0f, 1.0f, 1.0f);
	float lightIntensity = 1.0f;

	// Half-Lambert
	float NdotL = dot(normalize(input.normal), -lightDir);
	float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
	diffuse = baseColor * lightColor * cos * lightIntensity;

	// Blinn-Phong Specular
	float shininess = 20.0f;
	float3 toEye = normalize(float3(0.0f, 0.0f, -1.0f)); // シンプルに手前を向く
	float3 halfVector = normalize(-lightDir + toEye);
	float NDotH = dot(normalize(input.normal), halfVector);
	float specularPow = pow(saturate(NDotH), shininess);
	specular = lightColor * lightIntensity * specularPow * float3(0.3f, 0.3f, 0.3f); // 控えめなスペキュラ

	PixelShaderOutput output;
	output.color.rgb = diffuse + specular;
	output.color.a = input.color.a;

	return output;
}