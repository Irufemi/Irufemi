#pragma once

#include "CameraForGPU.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "AreaLight.h"
#include <cstdint>

/**
 * @struct PerFrameData
 * @brief フレーム全体で共有されるデータ構造体 (Camera, Timeなど)
 */
struct PerFrameData {
    CameraForGPU camera;        //!< カメラ情報 (view, projection, worldPosition)
    float time;                 //!< フレーム経過時間 (秒)
    float deltaTime;            //!< フレーム差分時間 (秒)
    
    // [Bindless] 各種グローバルリソースのインデックス
    uint32_t envMapIndex;       //!< 環境マップ (TextureCube, space2)
    uint32_t shadowMapIndex;    //!< シャドウマップ (Texture2D, space3)
    uint32_t depthMapIndex;     //!< デプスマップ (Texture2D, space3)
    uint32_t padding[3];        //!< 16バイトアライメント用パディング
};
