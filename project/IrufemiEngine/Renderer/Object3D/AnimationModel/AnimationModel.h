#pragma once

#include "Engine/Core/Math/Matrix4x4.h"
#include "Engine/Core/Math/Transform.h"
#include "Resource/Model/Data/AnimationTransform.h"
#include "Renderer/TransformationMatrix.h"
#include "../../../Engine/Core/Math/Vector2.h"
#include "../../../Engine/Core/Math/Vector3.h"
#include "../../../Engine/Core/Math/Vector4.h"
#include "../../../Engine/Core/Math/Transform.h"
#include "Engine/Graphics/DirectX/ConstantBuffer.h"
#include "Resource/Model/Data/Animation.h"
#include "Resource/Model/Data/NodeAnimation.h"
#include "Resource/Model/Data/ModelData.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Resource/Model/Data/Skeleton.h"
#include "Resource/Model/Data/SkinCluster.h"
#include "Renderer/Object3D/Object3DResource.h"
#include "Engine/Graphics/Data/Material.h"
#include "Engine/Manager/IComputeTask.h"
#include <d3d12.h>
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <wrl.h>

// 蜑肴婿螳｣險
class Camera;
class IrufemiEngine;
class SphereRegion;
class Line3DRegion;
struct ManagedModel;
struct ObjMaterial; 
struct Material;


class AnimationModel : public IComputeTask {
public: // 繝｡繝ｳ繝宣未謨ｰ

    AnimationModel();
    ~AnimationModel();

    void DispatchCompute() override;

    void Initialize(Camera* camera, const std::string& filename);

    void Update();

    void Draw();

    void Debug(const char* objName = " ");

    // 謠冗判逕ｨ縺ｮ螟画鋤陦悟・繝ｪ繧ｽ繝ｼ繧ｹ縺ｮGPU繧｢繝峨Ξ繧ｹ繧貞叙蠕・
    D3D12_GPU_VIRTUAL_ADDRESS GetTransformationGpuAddress() const {
        return transformationBuffer_.GetGPUVirtualAddress(BaseResource::GetDirectXCommon()->GetFrameIndex());
    }

private: // 繝｡繝ｳ繝宣未謨ｰ(蜀・Κ繝倥Ν繝・

    void UpdateMaterials();

    void UpdateAnimation();

    void InitializeResources();

public: // 繧ｲ繝・ち繝ｼ繝ｻ繧ｻ繝・ち繝ｼ
    // 謖・ｮ壹＠縺溘う繝ｳ繝・ャ繧ｯ繧ｹ縺ｮ繝｡繝・す繝･縺ｮ繝槭ユ繝ｪ繧｢繝ｫ繧貞叙蠕・隱ｭ縺ｿ蜿悶ｊ蟆ら畑)
    const ObjMaterial* GetMaterial(size_t meshIndex) const;
    // 謖・ｮ壹＠縺溘う繝ｳ繝・ャ繧ｯ繧ｹ縺ｮ繝｡繝・す繝･縺ｮ繝槭ユ繝ｪ繧｢繝ｫ繧貞叙蠕・譖ｸ縺崎ｾｼ縺ｿ蜿ｯ閭ｽ)
    ObjMaterial* GetMaterial(size_t meshIndex);

    void SetColor(const Vector4& color) { color_ = color; isDirty_ = true; }
    const Vector4& GetColor() const { return color_; }

    void SetTranslate(const Vector3& translate) { transform_.translate = translate; isDirty_ = true; }
    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; isDirty_ = true; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; isDirty_ = true; }
    void SetTransform(const Transform& transform) { transform_ = transform; isDirty_ = true; }
    const Transform& GetTransform() const { return transform_; }

    void SetEnvironmentCoefficient(float coefficient) { environmentCoefficient_ = coefficient; isDirty_ = true; }
    float GetEnvironmentCoefficient() const { return environmentCoefficient_; }

    void SetLightingModeOverride(int32_t mode) { lightingModeOverride_ = mode; isDirty_ = true; }
    void SetUseClampSamplerOverride(int32_t useClamp) { useClampSamplerOverride_ = useClamp; isDirty_ = true; }
    void SetEnableLightingOverride(int32_t enable) { enableLightingOverride_ = enable; isDirty_ = true; }


    static void SetIrufemiEngine(IrufemiEngine* engine) { engine_ = engine; }
    void SetCullingEnabled(bool enabled) { isCullingEnabled_ = enabled; }
    bool IsCullingEnabled() const { return isCullingEnabled_; }

private: // 繝｡繝ｳ繝仙､画焚
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

    // --- 謠冗判繝ｪ繧ｽ繝ｼ繧ｹ (譁ｰ繧｢繝ｼ繧ｭ繝・け繝√Ε) ---
    std::vector<std::unique_ptr<Object3DResource>> meshResources_;

    // 螟画鋤陦悟・逕ｨ繝ｪ繧ｽ繝ｼ繧ｹ (蜈ｨ繝｡繝・す繝･蜈ｱ譛・
    ConstantBuffer<TransformationMatrix> transformationBuffer_;
    std::map<std::string, Matrix4x4> nodeWorldMatrices_;

    Camera* camera_ = nullptr;

    static IrufemiEngine* engine_;

    Skeleton skeleton_;

    SkinCluster skinCluster_;

    // 繝弱・繝峨い繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ逕ｨ縺ｮ蝗ｺ譛窺atrix
    Matrix4x4 localMatrix_;

    Matrix4x4 worldMatrix_;

    Animation animation_;

    float animationTime_ = 0.0f;

    // --- 霑ｽ蜉・夐未遽陦ｨ遉ｺ逕ｨ縺ｮ繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ謠冗判讖滓ｧ・---
    std::unique_ptr<SphereRegion> jointSpheres_;
    std::unique_ptr<Line3DRegion> boneLines_;

    // 陦悟・譖ｴ譁ｰ縺ｮ譛驕ｩ蛹也畑
    bool isDirty_ = true;
    bool isCullingEnabled_ = true;
    Matrix4x4 lastViewMatrix_ = {};
    Matrix4x4 lastProjectionMatrix_ = {};

    void MarkAsDirty() {
        for(int i=0; i<kMaxFramesInFlight; ++i) isDirtyBuffer_[i] = true;
    }

private:
    bool isDirtyBuffer_[kMaxFramesInFlight] = {true, true, true};

    void SyncBeforeDraw();

    std::string filename_;
};