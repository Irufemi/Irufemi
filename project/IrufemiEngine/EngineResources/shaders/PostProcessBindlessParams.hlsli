#pragma once
struct PostProcessBindlessParams {
    uint32_t mainTextureIndex;
    uint32_t extraTextureIndex;
    uint32_t maskTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t materialTextureIndex;
    uint32_t velocityTextureIndex;
    uint32_t padding[2];
};
ConstantBuffer<PostProcessBindlessParams> gBindlessParams : register(b1);

#define gTexture gTextures[gBindlessParams.mainTextureIndex]
#define gExtraTexture gTextures[gBindlessParams.extraTextureIndex]
#define gMaskTexture gTextures[gBindlessParams.maskTextureIndex]
#define gNormalTexture gTextures[gBindlessParams.normalTextureIndex]
#define gMaterialTexture gTextures[gBindlessParams.materialTextureIndex]
#define gVelocityTexture gTextures[gBindlessParams.velocityTextureIndex]
