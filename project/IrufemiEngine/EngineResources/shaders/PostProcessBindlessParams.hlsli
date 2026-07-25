#pragma once
struct PostProcessBindlessParams {
    uint32_t mainTextureIndex;
    uint32_t extraTextureIndex;
    uint32_t maskTextureIndex;
    uint32_t padding;
};
ConstantBuffer<PostProcessBindlessParams> gBindlessParams : register(b1);

#define gTexture gTextures[gBindlessParams.mainTextureIndex]
#define gExtraTexture gTextures[gBindlessParams.extraTextureIndex]
#define gMaskTexture gTextures[gBindlessParams.maskTextureIndex]
