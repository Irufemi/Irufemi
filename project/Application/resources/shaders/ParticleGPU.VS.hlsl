#include "ParticleGPU.hlsli"

StructuredBuffer<Particle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);
ConstantBuffer<GPUParticleEmitter> gEmitter : register(b6); // Special Slot

struct VertexShaderInput
{
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
};

// 回転行列の作成 (XYZ)
float4x4 MakeRotationMatrix(float3 rotate)
{
    float3 c = cos(rotate);
    float3 s = sin(rotate);

    float4x4 mX = { 1, 0, 0, 0, 0, c.x, s.x, 0, 0, -s.x, c.x, 0, 0, 0, 0, 1 };
    float4x4 mY = { c.y, 0, -s.y, 0, 0, 1, 0, 0, s.y, 0, c.y, 0, 0, 0, 0, 1 };
    float4x4 mZ = { c.z, s.z, 0, 0, -s.z, c.z, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

    return mul(mZ, mul(mX, mY));
}

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) 
{
	VertexShaderOutput output;
	Particle particle = gParticles[instanceId];
	
    float4x4 worldMatrix;
    
    if (gEmitter.isBillboard != 0)
    {
        // Z軸回転の行列を作成
        float c = cos(particle.rotation.z);
        float s = sin(particle.rotation.z);
        float4x4 rotZ = {
             c, s, 0, 0,
            -s, c, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1
        };
        
        // スケール行列の作成
        float4x4 scaleMatrix = {
            particle.scale.x, 0, 0, 0,
            0, particle.scale.y, 0, 0,
            0, 0, particle.scale.z, 0,
            0, 0, 0, 1
        };

        // スケール -> Z軸回転 -> ビルボード（カメラ向き）の順に行列を合成
        worldMatrix = mul(mul(scaleMatrix, rotZ), gPerView.billboardMatrix);
    }
    else
    {
        // 3D回転 (SRT)
        float4x4 rotateMatrix = MakeRotationMatrix(particle.rotation);
        float4x4 scaleMatrix = {
            particle.scale.x, 0, 0, 0,
            0, particle.scale.y, 0, 0,
            0, 0, particle.scale.z, 0,
            0, 0, 0, 1
        };
        worldMatrix = mul(scaleMatrix, rotateMatrix);
    }
    
	worldMatrix[3].xyz = particle.translate;
    
	output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
	
    // UV アニメーション (テクスチャアトラス)
    float2 uv = input.texcoord;
    uint totalFrames = gEmitter.atlasRows * gEmitter.atlasCols;
    if (totalFrames > 1)
    {
        float t = saturate(particle.currentTime / particle.lifeTime);
        uint frameIndex = (uint)(t * (float)totalFrames);
        frameIndex = min(frameIndex, totalFrames - 1);
        
        uint row = frameIndex / gEmitter.atlasCols;
        uint col = frameIndex % gEmitter.atlasCols;
        
        float2 frameSize = 1.0f / float2(gEmitter.atlasCols, gEmitter.atlasRows);
        uv = (uv + float2(col, row)) * frameSize;
    }
    output.texcoord = uv;
	output.color = particle.color;
	return output;
}