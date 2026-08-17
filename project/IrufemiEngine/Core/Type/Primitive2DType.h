#pragma once


namespace Irufemi {
/**
 * @enum Primitive2DType
 * @brief 2Dプリミティブの形状タイプ
 */
enum class Primitive2DType {
    Rect,       //!< 四角形
    Triangle,   //!< 三角形
    Circle,     //!< 円（正多角形）
    Ring,       //!< ドーナツ状の円
    Line        //!< 線分
};

} // namespace Irufemi
