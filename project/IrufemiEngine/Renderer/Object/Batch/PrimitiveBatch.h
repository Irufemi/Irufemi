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

    static void SetPrimitiveManager(PrimitiveManager* manager) { primitiveManager_ = manager; }

    void Draw() override;

protected:
    inline static PrimitiveManager* primitiveManager_ = nullptr;
    float GetBoundingSphereRadius() const override;

private:
    void EnsureMaterialResources();
    void EnsureSharedTexture(const std::string& textureName);
    
private:
    Irufemi::PrimitiveType type_ = Irufemi::PrimitiveType::Sphere;
    bool isCustomPrimitive_ = false; // リングなどの個別パラメータを使用するか
    PrimitiveResource customPrimitiveResource_; // カスタム用のリソース
};
