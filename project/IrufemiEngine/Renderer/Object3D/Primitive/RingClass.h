#pragma once
#include "../../Core/IRenderable.h"
#include "Engine/Manager/PrimitiveManager.h"
#include <cstdint>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <string>

#include "Engine/Graphics/Camera/Camera.h"
#include "Renderer/Object3D/Object3DResource.h"

// 前方宣言
class TextureManager;
class DrawManager;
class DebugUI;

class RingClass : public IRenderable {
protected:
    // 中心位置
    Vector3 center_{ 0.0f, 0.0f, 0.0f };

    // Ring生成用パラメータ
    RingParams ringParams_{};

    // D3D12リソース
    std::unique_ptr<Object3DResource> resource_ = nullptr;

    int selectedTextureIndex_ = 0;

    // ポインタ参照(静的)
    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;

    static class IrufemiEngine* engine_;

public:
    RingClass() = default;
    ~RingClass() = default;

    void Initialize(const RingParams& params = RingParams(), const std::string& textureName = "resources/uvChecker.png");

    void Update();
    void SyncBeforeDraw() override;
    void Draw() override;
    void Debug(const char* name = "Ring");

    // バッファの再生成
    void RebuildResource();

    // Getters / Setters
    Object3DResource* GetD3D12Resource() { return resource_.get(); }
    void SetPosition(const Vector3& c) { center_ = c; isDirty_ = true; }
    void SetRotate(const Vector3& rot) { if (resource_) resource_->transform_.rotate = rot; isDirty_ = true; }
    void SetScale(const Vector3& scale) { if (resource_) resource_->transform_.scale = scale; isDirty_ = true; }
    void SetColor(const Vector4& color) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->color = color; }
    void SetAlphaReference(float alphaRef) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->alphaReference = alphaRef; }
    void SetUseClampSampler(int32_t useClamp) { if (resource_ && resource_->GetMaterialData()) resource_->GetMaterialData()->useClampSampler = useClamp; }

    static void SetTextureManager(TextureManager* texM) { textureManager_ = texM; }
    static void SetDrawManager(DrawManager* drawM) { drawManager_ = drawM; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetEngine(class IrufemiEngine* engine) { engine_ = engine; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }
    void SetCastShadows(bool cast) { castShadows_ = cast; }
    bool GetCastShadows() const { return castShadows_; }

private:
    // 行列更新の最適化用
    bool isDirty_ = true;
    bool isCullingEnabled_ = false; // デフォルトは無効（Ringの境界球計算が複雑なため）
    bool castShadows_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
};
