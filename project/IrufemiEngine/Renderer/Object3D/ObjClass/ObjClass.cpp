#include "ObjClass.h"
#include <filesystem>
#include <algorithm>
#include <Windows.h>
#include "Engine/Core/Math/Math.h"
#include "Resource/Texture/TextureManager.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Resource/Model/ModelManager.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"

// 鬮ｱ蜥丞飭郢晢ｽ｡郢晢ｽｳ郢昜ｻ呻ｽｮ螟ゑｽｾ・ｩ
TextureManager* ObjClass::textureManager_ = nullptr;
DrawManager* ObjClass::drawManager_ = nullptr;
DebugUI* ObjClass::ui_ = nullptr;
ModelManager* ObjClass::modelManager_ = nullptr;

ObjClass::~ObjClass() {
}

void ObjClass::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;

    assert(modelManager_ && "ObjClass::Initialize: ModelManager is not set.");
    // 鬮ｱ讒ｫ驟碑ｭ帶ｺ倥帝坡・ｭ邵ｺ・ｿ髴趣ｽｼ邵ｺ・ｿ郢ｧ蟶晏ｹ戊沂荵晢ｼ邵ｲ竏墅鍋ｹｧ・､郢晢ｽｳ郢ｧ・ｹ郢晢ｽｬ郢昴・繝ｩ郢ｧ蛛ｵ繝ｶ郢晢ｽｭ郢昴・縺醍ｸｺ蜉ｱ竊醍ｸｺ繝ｻ
    managedModel_ = modelManager_->GetModelAsync(filename);
    
    // Status邵ｺ骰ｬoaded邵ｺ・ｧ邵ｺ繧・ｽ檎ｸｺ・ｰ騾ｶ・ｴ邵ｺ・｡邵ｺ・ｫ陋ｻ譎・ｄ陋ｹ謔ｶ・帝圦・ｦ邵ｺ・ｿ郢ｧ繝ｻ
    auto status = managedModel_->status.load();
    if (status == ManagedModel::LoadingStatus::Loaded && managedModel_->cpuModel) {
        InitializeResources();
    }
}

void ObjClass::InitializeResources() {
    if (!managedModel_ || !managedModel_->cpuModel) {
        return;
    }

    // 陞溽判驪､髯ｦ謔溘・郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ・ｮ騾墓ｻ薙・邵ｺ・ｨ郢晄ｧｭ繝｣郢昴・(陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･陷茨ｽｱ隴幄・逡・
    assert(drawManager_ && "DrawManager is not set. Cannot get DirectXCommon.");
    transformationBuffer_.Initialize(drawManager_->GetDxCommon());

    // 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ陜暦ｽｺ隴帛ｳｨ繝ｻ陷ｷ繝ｻﾎ鍋ｹ昴・縺咏ｹ晢ｽ･騾包ｽｨ郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ郢ｧ蝣､蜃ｽ隰後・
    meshResources_.clear();
    for (size_t i = 0; i < managedModel_->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        
        // 陞溷､慚夂ｸｺ・ｮ陞溽判驪､髯ｦ謔溘・郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ郢ｧ雋楪貅ｽ逡・
        res->SetExternalTransformationBuffer(&transformationBuffer_);
        
        // 郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･陜暦ｽｺ隴帛ｳｨ繝ｻ View 郢ｧ螳夲ｽｨ・ｭ陞ｳ繝ｻ
        const auto& gpuMesh = managedModel_->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        // 郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ驕ｲ蟲ｨ繝ｻ騾墓ｻ薙・
        res->CreateResource();
        
        // 陋ｻ譎・ｄ郢昴・縺醍ｹｧ・ｹ郢昶・ﾎ慕ｹ昜ｸ莞ｦ郢晏ｳｨﾎ晉ｹｧ雋槭・隴帛ｳｨ繝ｧ郢晢ｽｼ郢ｧ・ｿ邵ｺ荵晢ｽ臥ｹｧ・ｳ郢晄鱒繝ｻ
        const auto& gpuMaterial = (i < managedModel_->gpuMaterials.size()) ? managedModel_->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }

        meshResources_.push_back(std::move(res));
    }

    // 陋ｻ譎丞ｱ填pdate郢ｧ雋樔ｻ也ｹｧ阮吶堤ｸｺ鄙ｫ・･
    Update();
}

void ObjClass::Update() {
    if (!managedModel_ || !camera_) return;

    // 鬮ｱ讒ｫ驟碑ｭ帶ｺ佩溽ｹ晢ｽｼ郢晏ｳｨ窶ｲ驍ｨ繧・ｽ冗ｸｺ・｣邵ｺ・ｦ邵ｺ繝ｻ・檎ｸｺ・ｰ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･郢ｧ蜻茨ｽｧ迢暦ｽｯ蟲ｨ笘・ｹｧ繝ｻ(鬩輔・・ｻ・ｶ陋ｻ譎・ｄ陋ｹ繝ｻ
    if (managedModel_->status.load() == ManagedModel::LoadingStatus::Loaded && meshResources_.empty()) {
        InitializeResources();
    }

    // 邵ｺ・ｾ邵ｺ・ｰ郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ譴ｧ・ｺ髢・咏ｸｺ・ｧ邵ｺ髦ｪ窶ｻ邵ｺ繝ｻ竊醍ｸｺ繝ｻ・ｰ・ｴ陷ｷ蛹ｻ繝ｻ郢ｧ・ｹ郢ｧ・ｭ郢昴・繝ｻ
    if (meshResources_.empty()) return;

    // 郢ｧ・ｪ郢晄じ縺夂ｹｧ・ｧ郢ｧ・ｯ郢昜ｺ･繝ｻ闖ｴ阮吶・郢晢ｽｯ郢晢ｽｼ郢晢ｽｫ郢晁歓・｡謔溘・郢ｧ螳夲ｽｨ閧ｲ・ｮ繝ｻ
    transformationMatrix_.world = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    // rootNode邵ｺ・ｮ髯ｦ謔溘・郢ｧ蟶昶・騾包ｽｨ(郢晢ｽ｢郢昴・ﾎ晉ｹ昴・繝ｻ郢ｧ・ｿ邵ｺ・ｫ鬮ｫ荳ｻ・ｱ・､隲繝ｻ・ｰ・ｱ邵ｺ蠕娯旺郢ｧ蠕後・)
    if (managedModel_->cpuModel) {
        transformationMatrix_.world = managedModel_->cpuModel->rootNode.localMatrix * transformationMatrix_.world;
    }

    // 雎墓・・ｷ螢ｼ・､逕ｻ驪､騾包ｽｨ邵ｺ・ｮ鬨ｾ繝ｻ・ｻ・｢驗ゑｽｮ髯ｦ謔溘・
    Matrix4x4 worldForNormal = transformationMatrix_.world;
    worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
    worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
    transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

    // 郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ隲繝ｻ・ｰ・ｱ郢ｧ遶ｪPU邵ｺ・ｸ髴・ｽ｢鬨ｾ繝ｻ
    UpdateMaterials();
    
    MarkAsDirty();
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
}

void ObjClass::SyncBeforeDraw() {
    uint32_t frameIndex = drawManager_->GetDxCommon()->GetFrameIndex();
    if (isDirtyBuffer_[frameIndex]) {
        transformationBuffer_.Update(transformationMatrix_, frameIndex);
        isDirtyBuffer_[frameIndex] = false;
    }
    
    // 陷ｷ繝ｻﾎ鍋ｹ昴・縺咏ｹ晢ｽ･邵ｺ・ｮ郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ驕ｲ蟲ｨ繝ｻ隴厄ｽｴ隴・ｽｰ
    for (auto& res : meshResources_) {
        res->SyncBeforeDraw();
    }
}

#include "../../../Engine/Core/Math/Geometry/Collision.h"
#include "../../../Engine/Core/Shape/Sphere.h"

void ObjClass::Draw() {
    if (!managedModel_ || !drawManager_ || !camera_) {
        return;
    }

    // 郢ｧ・ｫ郢晢ｽ｡郢晢ｽｩ邵ｺ・ｮ髯ｦ謔溘・邵ｺ謔滂ｽ､逕ｻ蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ邵ｲ竏壹′郢晄じ縺夂ｹｧ・ｧ郢ｧ・ｯ郢晞メ繝ｻ闖ｴ阮吮ｲ陞溽判蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ郢昶・縺臥ｹ昴・縺・
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    // --- 邵ｲ蜊・ｿ・ｽ陷会｣ｰ邵ｲ隨ｬ邱帝包ｽｻ騾ｶ・ｴ陷鷹亂繝ｻ郢晁・繝｣郢晁ｼ斐＜陷ｷ譴ｧ謔・---
    SyncBeforeDraw();

    // 髫募､懷ｹ陷ｿ・ｰ郢ｧ・ｫ郢晢ｽｪ郢晢ｽｳ郢ｧ・ｰ
    if (isCullingEnabled_ && managedModel_->cpuModel) {
        const Sphere& modelSphere = managedModel_->cpuModel->boundingSphere;

        // 郢晢ｽｯ郢晢ｽｼ郢晢ｽｫ郢晁・・ｩ・ｺ鬮｢阮吶・陟・・髦憺・・・帝坎閧ｲ・ｮ繝ｻ
        Sphere worldSphere;
        worldSphere.center = Math::Transform(modelSphere.center, transformationMatrix_.world);

        // 郢ｧ・ｹ郢ｧ・ｱ郢晢ｽｼ郢晢ｽｫ邵ｺ・ｮ隴崢陞滂ｽｧ陋滂ｽ､郢ｧ蟶昶・騾包ｽｨ邵ｺ蜉ｱ窶ｻ陷企宦・ｾ繝ｻ・定棔逕ｻ驪､
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        worldSphere.radius = modelSphere.radius * maxScale * 1.1f; // 10% 郢晄ｧｭ繝ｻ郢ｧ・ｸ郢晢ｽｳ

        // 陋ｻ・､陞ｳ繝ｻ
        if (!Collision::IsCollision(camera_->GetFrustum(), worldSphere)) {
            return; // 隰蜀怜愛郢ｧ・ｹ郢ｧ・ｭ郢昴・繝ｻ
        }
    }

    // 郢晢ｽ｢郢昴・ﾎ晁怙繝ｻ繝ｻ陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･郢ｧ蜻育ｷ帝包ｽｻ
    for (auto& res : meshResources_) {
        drawManager_->DrawStandard3D(res.get());
    }
}

void ObjClass::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("Obj: ") + objName;
    ImGui::Begin(name.c_str());
    DebugTab();
    ImGui::End();
#endif
}

void ObjClass::DebugTab() {
#if defined USE_IMGUI
    if (ui_) {
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);
        ui_->DebugTransform(transform_);
        ImGui::ColorEdit4("Color", &color_.x); // 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｫ郢晢ｽｩ郢晢ｽｼ郢ｧ蝣､・ｷ・ｨ鬮ｮ繝ｻ
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##OcOverrides");

        // ImGui邵ｺ・ｧ郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ郢ｧ蝣､・ｷ・ｨ鬮ｮ繝ｻ
        if (managedModel_ && managedModel_->cpuModel) {
            for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
                std::string materialLabel = "Mesh " + std::to_string(i) + " Material";
                if (ImGui::TreeNode(materialLabel.c_str())) {
                    ObjMaterial* mat = GetMaterial(i);
                    if (mat) {
                        // unique_id 郢ｧ蜻茨ｽｸ・｡邵ｺ蜉ｱ窶ｻ郢ｧ・ｳ郢晢ｽｳ郢晏現ﾎ溽ｹ晢ｽｼ郢晢ｽｫID邵ｺ・ｮ髯ｦ譎会ｽｪ竏夲ｽ帝ｩ包ｽｿ邵ｺ莉｣・・
                        std::string unique_id = "##" + std::to_string(i);
                        ui_->DebugObjMaterial(mat, unique_id.c_str());

                        // 郢昴・縺醍ｹｧ・ｹ郢昶・ﾎ暮ｩ包ｽｸ隰壹・
                        // 雎包ｽｨ隲｢謫ｾ・ｼ螢ｹ・・ｸｺ・ｮ鬩幢ｽｨ陋ｻ繝ｻ繝ｻObjClass邵ｺ蠕後Θ郢ｧ・ｯ郢ｧ・ｹ郢昶・ﾎ慕ｸｺ・ｮ郢ｧ・､郢晢ｽｳ郢昴・繝｣郢ｧ・ｯ郢ｧ・ｹ郢ｧ蜑・ｽｿ譎・亜邵ｺ蜷ｶ・玖脂諷包ｽｵ繝ｻ竏ｩ邵ｺ蠕娯・邵ｺ繝ｻ竊定楜謔溘・邵ｺ・ｫ邵ｺ・ｯ隶匁ｺｯ繝ｻ邵ｺ蜉ｱ竏ｪ邵ｺ蟶呻ｽ鍋ｸｲ繝ｻ
                        // 闔臥ｿｫ繝ｻUI邵ｺ・ｮ邵ｺ・ｿ髯ｦ・ｨ驕会ｽｺ邵ｺ蜉ｱ竏ｪ邵ｺ蜷ｶﾂ繝ｻ
                        int tempIndex = 0; // 郢敖郢晄ｺ倥・
                        // ui_->DebugTexture(...)
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
#endif
}

size_t ObjClass::GetMeshCount() const {
    if (managedModel_ && managedModel_->cpuModel) {
        return managedModel_->cpuModel->meshes.size();
    }
    return 0;
}

const ObjMaterial* ObjClass::GetMaterial(size_t meshIndex) const {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

ObjMaterial* ObjClass::GetMaterial(size_t meshIndex) {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

void ObjClass::SetEnableLightingToAllMeshes(bool enable) {
    enableLightingOverride_ = enable ? 1 : 0;
    isDirty_ = true;
}

void ObjClass::SetAlpha(float alpha) {
    color_.w = alpha;
    isDirty_ = true;
}

void ObjClass::SetColor(const Vector4& color) {
    color_ = color;
    isDirty_ = true;
}

void ObjClass::UpdateMaterials() {
    if (!managedModel_ || !managedModel_->cpuModel || meshResources_.empty()) {
        return;
    }

    // 陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･邵ｺ・ｮ郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ郢ｧ蜻亥ｳｩ隴・ｽｰ
    for (size_t i = 0; i < managedModel_->cpuModel->meshes.size(); ++i) {
        if (i >= meshResources_.size()) break;

        auto& res = meshResources_[i];
        if (!res->GetMaterialData()) continue;

        const ObjMaterial& cpuMat = managedModel_->cpuModel->meshes[i].material;
        Material* mappedData = res->GetMaterialData();

        // 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｫ郢晢ｽｩ郢晢ｽｼ邵ｺ・ｨ郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ郢ｧ・ｫ郢晢ｽｩ郢晢ｽｼ郢ｧ蜑・ｽｹ遉ｼ・ｮ繝ｻ
        mappedData->color.x = cpuMat.color.x * color_.x;
        mappedData->color.y = cpuMat.color.y * color_.y;
        mappedData->color.z = cpuMat.color.z * color_.z;
        mappedData->color.w = cpuMat.color.w * color_.w;
        if (mappedData->color.w <= 0.0f) { mappedData->color.w = 1.0f; }

        // 郢晢ｽｩ郢ｧ・､郢昴・縺・ｹ晢ｽｳ郢ｧ・ｰ邵ｺ・ｮ隴帷甥譟題ｿ･・ｶ隲ｷ繝ｻ(陋溷唱謖ｨ闕ｳ鬆大ｶ檎ｸｺ讎岩煤陷医・
        int32_t finalEnableLighting = (enableLightingOverride_ != -1) ? (enableLightingOverride_ == 1) : (cpuMat.enableLighting ? 1 : 0);
        mappedData->enableLighting = finalEnableLighting;

        mappedData->uvTransform = cpuMat.uvTransform;
        mappedData->metallic = cpuMat.metallic;
        mappedData->roughness = cpuMat.roughness;
        mappedData->hasTexture = !cpuMat.textureFilePath.empty();

        // 隴擾｣ｰ郢ｧ鬘假ｽｾ・ｼ邵ｺ・ｿ闖ｫ繧育・ (郢晢ｽ｢郢昴・ﾎ晁屐・､ * 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ闖ｫ繧育・)
        mappedData->environmentCoefficient = cpuMat.environmentCoefficient * environmentCoefficient_;

        // 郢晢ｽｩ郢ｧ・､郢昴・縺・ｹ晢ｽｳ郢ｧ・ｰ郢晢ｽ｢郢晢ｽｼ郢昴・(陋溷唱謖ｨ闕ｳ鬆大ｶ檎ｸｺ讎岩煤陷亥現ﾂ竏ｵ谺陞ｳ螢ｹ竊醍ｸｺ蜉ｱ竊醍ｹｧ蟲ｨﾎ皮ｹ昴・ﾎ晁屐・､邵ｲ竏墅帷ｹｧ・､郢昴・縺・ｹ晢ｽｳ郢ｧ・ｰ霎滂ｽ｡陷会ｽｹ邵ｺ・ｪ郢ｧ繝ｻ)
        if (lightingModeOverride_ != -1) {
            mappedData->lightingMode = lightingModeOverride_;
        } else {
            mappedData->lightingMode = finalEnableLighting ? cpuMat.lightingMode : 0;
        }

        // 郢ｧ・ｵ郢晢ｽｳ郢晏干ﾎ帷ｹ晢ｽｼ髫ｪ・ｭ陞ｳ繝ｻ(陋溷唱謖ｨ闕ｳ鬆大ｶ檎ｸｺ讎岩煤陷医・
        mappedData->useClampSampler = (useClampSamplerOverride_ != -1) ? useClampSamplerOverride_ : cpuMat.useClampSampler;
        
        // 郢ｧ・｢郢晢ｽｫ郢晁ｼ斐＜郢昴・縺帷ｹ晁ご逡鷹ｫ｢・ｾ陋滂ｽ､
        mappedData->alphaReference = cpuMat.alphaReference;
        res->MarkAsDirty();
    }
}