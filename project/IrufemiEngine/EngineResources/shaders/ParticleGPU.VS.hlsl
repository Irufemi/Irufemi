#include "ParticleGPU.hlsli"
#include "VertexData.hlsli"
#include "CullingUtility.hlsli"
#include "MathUtility.hlsli"

StructuredBuffer<Particle> gParticles : register(t0);
StructuredBuffer<ParticleSortData> gSortList : register(t1);
ConstantBuffer<PerView> gPerView : register(b0);

VertexShaderOutput main(VertexInput input, uint instanceId : SV_InstanceID) 
{
	VertexShaderOutput output;
    
    ParticleSortData sortData = gSortList[instanceId];
	Particle particle = gParticles[sortData.particleIndex];
    
    // 繧ｽ繝ｼ繝域凾縺ｫ莉倅ｸ弱＠縺歸epth縺瑚ｲ�縺ｮ蝣ｴ蜷医・豁ｻ繧薙〒縺・ｋ繝代・繝・ぅ繧ｯ繝ｫ縺ｪ縺ｮ縺ｧ繧ｫ繝ｪ繝ｳ繧ｰ
    if (sortData.depth < 0.0f)
    {
        CullInstanceByScale(particle.scale);
    }
	
    // 繝ｭ繝ｼ繧ｫ繝ｫ蠎ｧ讓吶↓繧ｹ繧ｱ繝ｼ繝ｫ繧帝←逕ｨ
    float3 localPos = input.position.xyz * particle.scale;
    float3 worldPos = float3(0, 0, 0);
    
    if (particle.billboardMode == 1) // Billboard
    {
        // Z霆ｸ蝗櫁ｻ｢繧帝←逕ｨ
        float4x4 rotZ = MakeRotateZMatrix(particle.rotation.z);
        float3 rotPos = mul(float4(localPos, 1.0f), rotZ).xyz;
        
        // 繝薙Ν繝懊・繝芽｡悟・繧帝←逕ｨ縺励※蟷ｳ陦檎ｧｻ蜍・
        worldPos = mul(float4(rotPos, 1.0f), gPerView.billboardMatrix).xyz + particle.translate;
    }
    else if (particle.billboardMode == 2) // Velocity Billboard
    {
        // 騾溷ｺｦ譁ｹ蜷代ン繝ｫ繝懊・繝・(Velocity Billboard)
        float3 dir = particle.velocity;
        float len = length(dir);
        if (len < 0.0001f)
        {
            dir = float3(0.0f, 1.0f, 0.0f);
        }
        else
        {
            dir /= len;
        }

        // 繧ｫ繝｡繝ｩ縺九ｉ繝代・繝・ぅ繧ｯ繝ｫ縺ｸ縺ｮ譁ｹ蜷代・繧ｯ繝医Ν
        float3 viewDir = normalize(particle.translate - gPerView.worldPosition);

        // 繝代・繝・ぅ繧ｯ繝ｫ縺ｮ蜿ｳ譁ｹ蜷托ｼ磯ｲ陦梧婿蜷代→隕也ｷ壹・繧ｯ繝医Ν縺ｮ螟也ｩ搾ｼ・
        float3 right = cross(dir, viewDir);
        float lenR = length(right);
        if (lenR < 0.0001f)
        {
            // 騾ｲ陦梧婿蜷代→隕也ｷ壹′蟷ｳ陦後↑蝣ｴ蜷医・縲∽ｻｻ諢上・蜿ｳ譁ｹ蜷代ｒ螳夂ｾｩ
            float3 upVec = abs(dir.y) < 0.999f ? float3(0,1,0) : float3(1,0,0);
            right = normalize(cross(dir, upVec));
        }
        else
        {
            right /= lenR;
        }

        // 繝代・繝・ぅ繧ｯ繝ｫ縺ｮ謇句燕・域ｳ慕ｷ夲ｼ画婿蜷・
        float3 normal = cross(right, dir);

        // 騾溷ｺｦ譁ｹ蜷代ン繝ｫ繝懊・繝牙屓霆｢陦悟・
        // Y霆ｸ縺碁ｲ陦梧婿蜷・(dir) 縺ｫ謨ｴ蛻励＠縲々霆ｸ縺悟承 (right) 縺ｫ謨ｴ蛻励＠縲〇霆ｸ縺梧焔蜑・(normal) 縺ｫ謨ｴ蛻励☆繧・
        float3x3 rotMatrix = {
            right.x,  right.y,  right.z,
            dir.x,    dir.y,    dir.z,
            normal.x, normal.y, normal.z
        };

        worldPos = mul(localPos, rotMatrix) + particle.translate;
    }
    else // 3D Rotation
    {
        // 3D蝗櫁ｻ｢ (SRT)
        float4x4 rotateMatrix = MakeRotateXYZMatrix(particle.rotation);
        worldPos = mul(float4(localPos, 1.0f), rotateMatrix).xyz + particle.translate;
    }
    
    // ViewProjection繧帝←逕ｨ縺励※譛邨ら噪縺ｪ鬆らせ蠎ｧ讓吶ｒ險育ｮ・
	output.position = mul(float4(worldPos, 1.0f), gPerView.viewProjection);
	
    // UV 繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ (繝・け繧ｹ繝√Ε繧｢繝医Λ繧ｹ)
    float2 uv = input.texcoord;
    uint atlasRows = (particle.atlasSize >> 16) & 0xFFFF;
    uint atlasCols = particle.atlasSize & 0xFFFF;
    uint totalFrames = max(1, atlasRows * atlasCols);
    if (totalFrames > 1)
    {
        float t = saturate(particle.currentTime / particle.lifeTime);
        uint frameIndex = (uint)(t * (float)totalFrames);
        frameIndex = min(frameIndex, totalFrames - 1);
        
        uint row = frameIndex / max(1, atlasCols);
        uint col = frameIndex % max(1, atlasCols);
        
        float2 frameSize = 1.0f / float2(max(1, atlasCols), max(1, atlasRows));
        uv = (uv + float2(col, row)) * frameSize;
    }
    output.texcoord = float4(uv, particle.translate.xy);
    output.timeRatio = saturate(particle.currentTime / max(particle.lifeTime, 0.0001f));
	output.color = input.color * particle.color;
	output.cameraNear = gPerView.cameraNear;
	output.cameraFar = gPerView.cameraFar;
	return output;
}
