#pragma once
struct PostProcessBindlessParams {
    uint32_t mainTextureIndex;
    uint32_t extraTextureIndex;
    uint32_t2 padding;
};
ConstantBuffer<PostProcessBindlessParams> gBindlessParams : register(b1);

#define gTexture gTextures[gBindlessParams.mainTextureIndex]
#define gExtraTexture gTextures[gBindlessParams.extraTextureIndex]
