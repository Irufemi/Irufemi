#pragma once

namespace Irufemi {

/**
 * @enum PrimitiveType
 * @brief 3Dプリミティブメッシュの形状タイプ
 */
enum class PrimitiveType {
    Triangle,  //!< 単一三角形（平面）
    Plane,     //!< 平面（板ポリゴン、UVマッピング対応）
    Cube,      //!< 直方体 / キューブ（各面法線付き）
    Cylinder,  //!< 円柱（側面および上下キャップ）
    Sphere,    //!< UV球（緯度経度分割メッシュ）
    Tetra,     //!< 正四面体（4面）
    Circle,    //!< 3D空間上の円盤
    Ring,      //!< 3D空間上のリング（円環）
    Skybox,    //!< 天球 / 背景描画用キューブ
    Cone,      //!< 円錐（底面キャップ付き）
    Torus,     //!< ドーナツ型（トーラス曲面）
    IcoSphere, //!< 正二十面体ベースの均等分割球（アイコスフィア）
    Grid,      //!< 床面デバッグ用グリッドメッシュ
    Octahedron //!< 正八面体（8面）
};
} // namespace Irufemi
