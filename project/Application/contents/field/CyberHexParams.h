#pragma once

#include "Engine/Core/Math/Vector4.h"

/**
 * @struct CyberHexParams
 * @brief CyberHex.PS.hlsl 専用のパラメータ構造体
 * HLSL側の定数バッファ (register b6) とメモリレイアウトを完全に一致させます。
 * アライメントは16バイト単位である必要があります。
 */
struct CyberHexParams {
    Vector4 edgeColor;        // 縁（発光）の色 (16 bytes)
    float edgeThickness;      // 縁の太さ (4 bytes)
    float baseBrightness;     // ベースの明るさ (4 bytes)
    float flickerAmplitude;   // 明滅の振幅 (4 bytes)
    float distortion;         // 空間の歪み具合 (4 bytes)

    float density;            // ヘキサゴンの密度 (4 bytes) (元 uvTransform.m[0][0])
    float animationSpeed;     // 浮き沈みアニメーション速度 (4 bytes) (元 uvTransform.m[1][1])
    float uvScrollX;          // UVスクロール速度 X (4 bytes)
    float uvScrollY;          // UVスクロール速度 Y (4 bytes)
    
    float mappingMode;        // マッピングモード (0.0=Triplanar, 1.0=Cylindrical Z軸) (4 bytes)
    float padding[3];         // 16バイトアライメント用パディング (12 bytes)
    // 合計 64 bytes (16 bytes * 4)
};
