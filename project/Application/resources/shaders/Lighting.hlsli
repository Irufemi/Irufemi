#include "./Material.hlsli"

// --- 構造体定義 ---

struct DirectionalLight {
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct PointLight {
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float radius;
    float decay;
    int32_t isActive;
    float padding;
};

struct SpotLight {
    float32_t4 color;
    float32_t3 position;
    float32_t intensity;
    float32_t3 direction;
    float32_t distance;
    float32_t decay;
    float32_t cosAngle;
    float32_t falloff;
    int32_t isActive;
    float32_t3 padding;
};

struct AreaLight {
    float4 color;
    float3 position;
    float intensity;
    float3 direction;
    float range;
    float2 size;
    int32_t isActive;
    float padding;
};

// --- 計算用コンテキスト ---

struct LightContext {
    float3 normal;
    float3 worldPosition;
    float3 toEye;
};

// --- ライティング計算関数 ---

/**
 * 拡散反射強度を計算する (Lambert / Half-Lambert)
 */
float CalculateDiffuseFactor(float3 normal, float3 lightDir, int mode) {
    float NdotL = dot(normal, -lightDir);
    if (mode == 1) { // Lambert
        return saturate(NdotL);
    } else if (mode == 2) { // Half-Lambert
        return pow(NdotL * 0.5f + 0.5f, 2.0f);
    }
    return 1.0f;
}

/**
 * 鏡面反射強度を計算する (Blinn-Phong)
 */
float CalculateSpecularFactor(float3 normal, float3 lightDir, float3 toEye, float shininess) {
    float3 halfVector = normalize(-lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    return pow(saturate(NdotH), shininess);
}

/**
 * 平行光源の計算
 */
void ApplyDirectionalLight(DirectionalLight light, Material material, LightContext context, inout float3 diffuseColor, inout float3 specularColor) {
    float diffuseFactor = CalculateDiffuseFactor(context.normal, light.direction, material.lightingMode);
    float specularFactor = CalculateSpecularFactor(context.normal, light.direction, context.toEye, material.shininess);
    
    diffuseColor += light.color.rgb * light.intensity * diffuseFactor;
    specularColor += light.color.rgb * light.intensity * specularFactor;
}

/**
 * 点光源の計算
 */
void ApplyPointLight(PointLight light, Material material, LightContext context, inout float3 diffuseColor, inout float3 specularColor) {
    if (light.isActive == 0) return;

    float3 lightDir = normalize(context.worldPosition - light.position);
    
    float diffuseFactor = CalculateDiffuseFactor(context.normal, lightDir, material.lightingMode);
    float specularFactor = CalculateSpecularFactor(context.normal, lightDir, context.toEye, material.shininess);

    diffuseColor += light.color.rgb * light.intensity * diffuseFactor;
    specularColor += light.color.rgb * light.intensity * specularFactor;
}

/**
 * スポットライトの計算
 */
void ApplySpotLight(SpotLight light, Material material, LightContext context, inout float3 diffuseColor, inout float3 specularColor) {
    if (light.isActive == 0) return;

    float3 lightDir = normalize(context.worldPosition - light.position);
    float d = length(context.worldPosition - light.position);
    
    // 距離減衰
    float attenuation = pow(saturate(1.0f - d / max(light.distance, 1e-5f)), light.decay);
    
    // 角度減衰
    float cosAngle = dot(lightDir, light.direction);
    float falloff = saturate((cosAngle - light.cosAngle) / (1.0f - light.cosAngle));
    
    float factor = attenuation * falloff;

    float diffuseFactor = CalculateDiffuseFactor(context.normal, lightDir, material.lightingMode);
    float specularFactor = CalculateSpecularFactor(context.normal, lightDir, context.toEye, material.shininess);

    diffuseColor += light.color.rgb * light.intensity * diffuseFactor * factor;
    specularColor += light.color.rgb * light.intensity * specularFactor * factor;
}

/**
 * エリアライトの計算
 */
void ApplyAreaLight(AreaLight light, Material material, LightContext context, inout float3 diffuseColor, inout float3 specularColor) {
    if (light.isActive == 0) return;

    float3 lightDir = light.direction; // エリアライトは方向固定の簡易実装と仮定
    float d = length(context.worldPosition - light.position);
    float attenuation = pow(saturate(1.0f - d / max(light.range, 1e-5f)), 1.0f);

    float diffuseFactor = CalculateDiffuseFactor(context.normal, lightDir, material.lightingMode);
    float specularFactor = CalculateSpecularFactor(context.normal, lightDir, context.toEye, material.shininess);

    diffuseColor += light.color.rgb * light.intensity * diffuseFactor * attenuation;
    specularColor += light.color.rgb * light.intensity * specularFactor * attenuation;
}
