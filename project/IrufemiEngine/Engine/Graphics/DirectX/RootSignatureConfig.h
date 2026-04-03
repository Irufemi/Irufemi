#pragma once
#include <d3d12.h>

/**
 * @enum RootSlot
 * @brief ルートパラメータのスロットインデックスを定義します。
 * @details HLSL側での register(bX) や register(tX) とは独立した、
 *          ID3D12GraphicsCommandList::SetGraphicsRoot... で使用するインデックスです。
 *          現在は既存の構成を維持するため、マジックナンバーをそのままEnum化しています。
 */
enum class RootSlot : UINT {
    Material = 0,         ///< マテリアル (register b0) - PS
    Transform = 1,        ///< 座標変換行列 (register b0) - VS
    Texture = 2,          ///< メインテクスチャ (register t0) - PS
    DirectionalLight = 3,  ///< 平行光源 (register b1) - PS
    InstancingData = 4,    ///< インスタンシング用 SRV (register t0) - VS
    Camera = 5,            ///< カメラ (register b2) - VS/PS
    PointLights = 6,       ///< 点光源 (register b3) - PS
    SpotLights = 7,        ///< スポットライト (register b4) - PS
    Unused8 = 8,           ///< 未使用 (register b6)
    Unused9 = 9,           ///< 未使用 (register b5)
    AreaLights = 10,       ///< エリアライト (register b7) - PS
    LineInstancing = 11,   ///< ライン用インスタンシング SRV (register t1) - VS
    EnvironmentMap = 12,   ///< 環境マップ (register t1) - PS
};
