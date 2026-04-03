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
    float padding[2];           //!< パディング
};

/**
 * @struct SceneLightData
 * @brief シーン内の全ての光源データ構造体
 */
struct SceneLightData {
    DirectionalLight directionalLight; //!< 平行光源
    
    static constexpr uint32_t MAX_POINT_LIGHTS = 4;
    PointLight pointLights[MAX_POINT_LIGHTS]; //!< 点光源リスト
    
    static constexpr uint32_t MAX_SPOT_LIGHTS = 4;
    SpotLight spotLights[MAX_SPOT_LIGHTS];   //!< スポットライトリスト
    
    static constexpr uint32_t MAX_AREA_LIGHTS = 4;
    AreaLight areaLights[MAX_AREA_LIGHTS];   //!< エリアライトリスト
};
