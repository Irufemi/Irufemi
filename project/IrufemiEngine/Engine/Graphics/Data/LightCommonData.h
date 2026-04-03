#pragma once

#include "DirectionalLight.h"
#include <cstdint>

/**
 * @struct LightCommonData
 * @brief ライトに関連する共通情報を格納する定数バッファ用構造体
 */
struct LightCommonData {
    DirectionalLight directionalLight; //!< 平行光源 (1体固定)
    uint32_t pointLightCount;          //!< 有効な点光源の数
    uint32_t spotLightCount;           //!< 有効なスポットライトの数
    uint32_t areaLightCount;           //!< 有効なエリアライトの数
    uint32_t padding;                  //!< パディング (16進アライメント用)
};
