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
    BindlessSRV = 2,      ///< フル Bindless用 SRVテーブル (register t0, space1-6) - ALL
    LightCommon = 3,      ///< ライト共通データ (register b1) - VS/PS
    // Instancing (t0, space0) や Special (b6) はバインド方法を維持するか定数に回すか、とりあえずそのままの枠は残す
    Instancing = 4,       ///< インスタンシング用 SRV (register t0, space0) - VS
    Camera = 5,           ///< カメラ/PerFrameData (register b2) - VS/PS
    Special = 6,          ///< 特殊用 (GSなど) (register b6) - ALL
    LineInstancing = 7,   ///< ライン用インスタンシング (register t1, space0) - VS
    Texture = 8,          ///< レガシーテクスチャ (register t0, space0) - PS
    EnvMap = 9,           ///< レガシー環境マップ (register t1, space0) - PS
    Lights = 10,          ///< レガシーライトSRVテーブル (register t2, t3, t4) - PS
    ShadowMap = 11,       ///< レガシーシャドウマップ (register t5) - PS
    DepthMap = 12,        ///< レガシーメイン深度マップ (register t6) - PS
};
