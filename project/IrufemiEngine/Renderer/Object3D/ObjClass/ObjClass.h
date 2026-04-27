#pragma once
#include <d3d12.h>
#include <string>
#include "../../../../Application/camera/Camera.h"
#include "../../TransformationMatrix.h"
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "../../../Engine/Core/Math/Transform.h"
#include "../../../Engine/Core/Math/Vector4.h"
#include "../../../Engine/Core/Math/Matrix4x4.h"
#include "../../../Resource/Model/Data/ObjModel.h"
#include "../Object3DResource.h"
#include "../../../Engine/Graphics/Data/Material.h"
#include "../../../Engine/Graphics/DirectX/ConstantBuffer.h"

// 蜑肴婿螳｣險
class TextureManager;
class DrawManager;
class DebugUI;
class ModelManager;
struct ManagedModel;
struct ObjMaterial;
struct Material;

//==========================
// obj縺碁・蟶・＆繧後※縺・ｋ繧ｵ繧､繝・
// https://quaternius.com/
// 菴ｿ逕ｨ縺吶ｋ蝣ｴ蜷医・繝ｩ繧､繧ｻ繝ｳ繧ｹ縺靴CO縺ｮ繧ゅ・繧貞茜逕ｨ縺吶ｋ
// https://creativecommons.org/publicdomain/zero/1.0/deed.ja
//==========================

/**
 * @class ObjClass
 * @brief 3D繝｢繝・Ν・・BJ/GLTF遲会ｼ峨・繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ繧呈緒逕ｻ繝ｻ邂｡逅・☆繧九け繝ｩ繧ｹ
 * @details ModelManager 縺九ｉ蜿門ｾ励＠縺溷・譛峨Δ繝・Ν繝・・繧ｿ繧貞盾辣ｧ縺励∝句挨縺ｮ菴咲ｽｮ繝ｻ蝗櫁ｻ｢繝ｻ諡｡邵ｮ繧・・繝・Μ繧｢繝ｫ險ｭ螳壹ｒ菫晄戟縺励∪縺吶・
 */
class ObjClass {
private:
    // 蜈ｱ譛峨Δ繝・Ν繝・・繧ｿ(CPU/GPU)
    std::shared_ptr<ManagedModel> managedModel_;

    // 繧ｪ繝悶ず繧ｧ繧ｯ繝亥・菴薙・Transform
    Transform transform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    TransformationMatrix transformationMatrix_{};
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f }; // 繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ繧ｫ繝ｩ繝ｼ
    float environmentCoefficient_ = 1.0f; // 繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ迺ｰ蠅・・繝・・菫よ焚
    int32_t lightingModeOverride_ = -1; // -1:菴ｿ逕ｨ縺励↑縺・ 0莉･荳・荳頑嶌縺・
    int32_t useClampSamplerOverride_ = -1; // -1:菴ｿ逕ｨ縺励↑縺・ 0莉･荳・荳頑嶌縺・
    int32_t enableLightingOverride_ = -1; // -1:菴ｿ逕ｨ縺励↑縺・ 0莉･荳・荳頑嶌縺・

    // 蜷・Γ繝・す繝･逕ｨ繝ｪ繧ｽ繝ｼ繧ｹ (繝槭ユ繝ｪ繧｢繝ｫ繧・らせView繧剃ｿ晄戟)
    std::vector<std::unique_ptr<Object3DResource>> meshResources_;

    // 螟画鋤陦悟・逕ｨ繝ｪ繧ｽ繝ｼ繧ｹ (蜈ｨ繝｡繝・す繝･縺ｧ蜈ｱ譛・
    ConstantBuffer<TransformationMatrix> transformationBuffer_;
    
    Camera* camera_ = nullptr;

    // 繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ蟇ｾ蠢懊ヵ繝ｩ繧ｰ
    bool hasAnimation_ = false;

    // 繧ｫ繝ｪ繝ｳ繧ｰ險ｭ螳・
    bool isCullingEnabled_ = true;
    bool isCulled_ = false;

    // 譖ｴ譁ｰ邂｡逅・
    bool isDirty_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};
    
    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};

    void SyncBeforeDraw();

    static TextureManager* textureManager_;
    static DrawManager* drawManager_;
    static DebugUI* ui_;
    static ModelManager* modelManager_;

    /**
     * @brief CPU蛛ｴ縺ｮ繝槭ユ繝ｪ繧｢繝ｫ繝・・繧ｿ繧竪PU繝ｪ繧ｽ繝ｼ繧ｹ縺ｸ霆｢騾√☆繧具ｼ亥・驛ｨ逕ｨ・・
     */
    void UpdateMaterials();

    /**
     * @brief 繝ｭ繝ｼ繝牙ｮ御ｺ・ｾ後↓繝｡繝・す繝･遲峨・繝ｪ繧ｽ繝ｼ繧ｹ繧呈ｧ狗ｯ峨☆繧具ｼ磯≦蟒ｶ蛻晄悄蛹厄ｼ・
     */
    void InitializeResources();

public: //繝｡繝ｳ繝宣未謨ｰ

    /**
     * @brief 繝・せ繝医Λ繧ｯ繧ｿ
     */
    ~ObjClass();

    /**
     * @brief 蛻晄悄蛹・
     * @param[in] camera 菴ｿ逕ｨ縺吶ｋ繧ｫ繝｡繝ｩ
     * @param[in] filename 繝｢繝・Ν繝輔ぃ繧､繝ｫ蜷搾ｼ・odelManager邨檎罰縺ｧ繝ｭ繝ｼ繝会ｼ・
     */
    void Initialize(Camera* camera, const std::string& filename = "plane.obj");

    /**
     * @brief 譖ｴ譁ｰ蜃ｦ逅・
     * @details 繝ｯ繝ｼ繝ｫ繝芽｡悟・縺ｮ險育ｮ励→螳壽焚繝舌ャ繝輔ぃ縺ｸ縺ｮ霆｢騾√ｒ陦後＞縺ｾ縺吶・
     */
    void Update();

    /**
     * @brief 謠冗判繧ｳ繝槭Φ繝峨・遨阪∩霎ｼ縺ｿ
     */
    void Draw();

    /**
     * @brief 繝・ヰ繝・げ逕ｨUI縺ｮ陦ｨ遉ｺ
     */
    void Debug(const char* objName = " ");

    /**
     * @brief 繝・ヰ繝・げ逕ｨ繧ｿ繝悶・陦ｨ遉ｺ
     */
    void DebugTab();

    /** @name Transform 謫堺ｽ・*/
    ///@{
    const Vector3& GetPosition() const { return transform_.translate; }
    void SetPosition(const Vector3& position) { transform_.translate = position; isDirty_ = true; }

    const Vector3& GetRotate() const { return transform_.rotate; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; isDirty_ = true; }
    void SetRotateX(const float& rotate) { transform_.rotate.x = rotate; isDirty_ = true; }
    void SetRotateY(const float& rotate) { transform_.rotate.y = rotate; isDirty_ = true; }
    void SetRotateZ(const float& rotate) { transform_.rotate.z = rotate; isDirty_ = true; }

    const Vector3& GetScale() const { return transform_.scale; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; isDirty_ = true; }
    const Transform& GetTransform() const { return transform_; }
    void SetTransform(const Transform& transform) { transform_ = transform; isDirty_ = true; }
    ///@}

    /** @name 陦悟・繝ｻ險育ｮ礼ｵ先棡縺ｮ蜿門ｾ・*/
    ///@{
    const TransformationMatrix& GetTransformationMatrix() const { return transformationMatrix_; }
    void SetTransformationMatrix(const TransformationMatrix& transformationMatrix) { transformationMatrix_ = transformationMatrix; }
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }
    ///@}

    /** @name 繝槭ユ繝ｪ繧｢繝ｫ繝ｻ螟冶ｦｳ縺ｮ謫堺ｽ・*/
    ///@{
    /**
     * @brief 繝｢繝・Ν縺梧戟縺､繝｡繝・す繝･謨ｰ繧貞叙蠕・
     */
    size_t GetMeshCount() const;

    /**
     * @brief 謖・ｮ壹＠縺溘う繝ｳ繝・ャ繧ｯ繧ｹ縺ｮ繝｡繝・す繝･縺ｮ繝槭ユ繝ｪ繧｢繝ｫ繧貞叙蠕・
     */
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    ObjMaterial* GetMaterial(size_t meshIndex);

    /**
     * @brief 縺吶∋縺ｦ縺ｮ繝｡繝・す繝･縺ｮ繝ｩ繧､繝・ぅ繝ｳ繧ｰ繧剃ｸ諡ｬ縺ｧ譛牙柑/辟｡蜉ｹ蛹悶☆繧・
     */
    void SetEnableLightingToAllMeshes(bool enable);

    /**
     * @brief 繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ蜈ｨ菴薙・繧｢繝ｫ繝輔ぃ蛟､繧定ｨｭ螳・
     */
    void SetAlpha(float alpha);

    /**
     * @brief 繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ蜈ｨ菴薙・濶ｲ繧定ｨｭ螳・
     */
    void SetColor(const Vector4& color);
    void SetEnvironmentCoefficient(float coefficient) { environmentCoefficient_ = coefficient; isDirty_ = true; }
    void SetLightingModeOverride(int32_t mode) { lightingModeOverride_ = mode; isDirty_ = true; }
    void SetUseClampSamplerOverride(int32_t useClamp) { useClampSamplerOverride_ = useClamp; isDirty_ = true; }
    void SetEnableLightingOverride(int32_t enable) { enableLightingOverride_ = enable; isDirty_ = true; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    ///@}

    /** @name 髱咏噪繝｡繝ｳ繝占ｨｭ螳夲ｼ医お繝ｳ繧ｸ繝ｳ蜀・Κ逕ｨ・・*/
    ///@{
    static void SetTextureManager(TextureManager* tm) { textureManager_ = tm; }
    static void SetDrawManager(DrawManager* dm) { drawManager_ = dm; }
    static void SetDebugUI(DebugUI* ui) { ui_ = ui; }
    static void SetModelManager(ModelManager* mm) { modelManager_ = mm; }
    ///@}
};

