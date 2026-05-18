#pragma once

#include <vector>
#include <memory>
#include "Engine/Core/Math/Math.h"

class IrufemiEngine;
struct AreaLight;

/**
 * @class DynamicArenaLight
 * @brief プレイヤーとエネミーの間に配置される、動的な闘技場照明（エリアライト）を管理するクラス
 */
class DynamicArenaLight {
public:
    DynamicArenaLight() = default;
    ~DynamicArenaLight() = default;

    /**
     * @brief ライトの生成とシーンのライトリストへの登録を行う
     * @param engine エンジンのポインタ
     * @param sceneAreaLights シーンが保持するエリアライトのリスト
     */
    void Initialize(IrufemiEngine* engine, std::vector<std::unique_ptr<AreaLight>>& sceneAreaLights);

    /**
     * @brief プレイヤーとボスの位置から、照明の座標・向き・シャドウマップを更新する
     * @param playerPos プレイヤーのワールド座標
     * @param enemyPos エネミーのワールド座標
     */
    void Update(const Vector3& playerPos, const Vector3& enemyPos);

private:
    IrufemiEngine* engine_ = nullptr;
    AreaLight* controlledLight_ = nullptr; ///< 制御対象のエリアライトの参照

    // --- 各種定数パラメーター ---
    static constexpr float kFixedLightHeight = 50.0f;       ///< 照明の固定の高さ
    static constexpr float kBaseIntensity = 6.0f;           ///< 基礎の明るさ
    static constexpr float kIntensityDistanceFactor = 0.1f; ///< 距離による明るさの増加率
    
    // エリアライト固有のパラメータ
    static constexpr float kLightSizeWidth = 80.0f;         ///< エリアライトの幅
    static constexpr float kLightSizeHeight = 80.0f;        ///< エリアライトの高さ
    static constexpr float kLightRangeDistanceFactor = 2.0f;///< 距離による影響範囲の倍率
    static constexpr float kMinLightRange = 150.0f;         ///< 最小影響範囲

    // シャドウマップ関連
    static constexpr float kMinShadowOrthoSize = 40.0f;     ///< シャドウマップの最小サイズ
    static constexpr float kShadowOrthoMargin = 20.0f;      ///< シャドウマップの余白
    static constexpr float kShadowOrthoDistanceFactor = 0.6f;
};
