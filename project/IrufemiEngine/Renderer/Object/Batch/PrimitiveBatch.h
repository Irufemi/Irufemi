#pragma once

#include "../../System/Core/BaseBatch.h"
#include "Engine/Manager/PrimitiveManager.h"

/**
 * @class PrimitiveBatch
 * @brief PrimitiveManager を利用した基本形状を描画する領域クラス
 */
class PrimitiveBatch : public BaseBatch {
public:
    PrimitiveBatch() = default;
    ~PrimitiveBatch() override = default;

    /**
     * @brief プリミティブ形状の領域を初期化する
     * @param type 生成する形状の種類
     * @param textureName 適用するテクスチャパス
     */
    void Initialize(Irufemi::PrimitiveType type, const std::string& textureName = "resources/uvChecker.png");

    /**
     * @brief リング形状専用の初期化
     */
    void InitializeRing(const RingParams& params, const std::string& textureName = "resources/uvChecker.png");

    /**
     * @brief PrimitiveManager を設定する。
     * @param[in] manager 設定する PrimitiveManager の値
     */
    static void SetPrimitiveManager(PrimitiveManager* manager) { primitiveManager_ = manager; }

    /**
     * @brief Draw を実行する。
     */
    void Draw() override;

protected:
    inline static PrimitiveManager* primitiveManager_ = nullptr;
    /**
     * @brief BoundingSphereRadius を取得する。
     * @return 取得された BoundingSphereRadius
     */
    float GetBoundingSphereRadius() const override;

private:
    /**
     * @brief EnsureMaterialResources を実行する。
     */
    void EnsureMaterialResources();
    /**
     * @brief EnsureSharedTexture を実行する。
     */
    void EnsureSharedTexture(const std::string& textureName);
    
private:
    Irufemi::PrimitiveType type_ = Irufemi::PrimitiveType::Sphere;
    bool isCustomPrimitive_ = false; // リングなどの個別パラメータを使用するか
    PrimitiveResource customPrimitiveResource_; // カスタム用のリソース
};
