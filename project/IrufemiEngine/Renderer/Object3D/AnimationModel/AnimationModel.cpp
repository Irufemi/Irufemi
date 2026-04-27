#include "AnimationModel.h"

#include "Engine/IrufemiEngine.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Graphics/DirectX/DirectXCommon.h"
#include "Engine/Manager/DrawManager.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Core/Math/Geometry/Frustum.h"
#include "Engine/Core/Math/Geometry/Collision.h"
#include "Engine/Core/Shape/Sphere.h"
#include "Resource/Model/ModelManager.h"
#include "Resource/Model/AnimationManager.h"
#include "Resource/Model/Data/ObjModel.h"
#include "Renderer/Region/Primitive/SphereRegion.h"
#include "Renderer/LineInstanced/LineClass.h"
#include "Application/camera/Camera.h"
#include <cmath>
#include <cassert>

// 鬮ｱ蜥丞飭郢晢ｽ｡郢晢ｽｳ郢昜ｻ呻ｽｮ螟ゑｽｾ・ｩ
IrufemiEngine* AnimationModel::engine_ = nullptr;

AnimationModel::AnimationModel() {}
AnimationModel::~AnimationModel() {
}

// 陋ｻ譎・ｄ陋ｹ繝ｻ
void AnimationModel::Initialize(Camera* camera, const std::string& filename) {
    camera_ = camera;
    filename_ = filename;

    assert(engine_ && "AnimationModel::Initialize: ModelManager is not set.");
    // 鬮ｱ讒ｫ驟碑ｭ帶ｺ倥帝坡・ｭ邵ｺ・ｿ髴趣ｽｼ邵ｺ・ｿ郢ｧ蟶晏ｹ戊沂荵晢ｼ邵ｲ竏墅鍋ｹｧ・､郢晢ｽｳ郢ｧ・ｹ郢晢ｽｬ郢昴・繝ｩ郢ｧ蛛ｵ繝ｶ郢晢ｽｭ郢昴・縺醍ｸｺ蜉ｱ竊醍ｸｺ繝ｻ
    managedModel_ = engine_->GetObjModelManager()->GetModelAsync(filename);

    // Status邵ｺ骰ｬoaded邵ｺ・ｧ邵ｺ繧・ｽ檎ｸｺ・ｰ騾ｶ・ｴ邵ｺ・｡邵ｺ・ｫ陋ｻ譎・ｄ陋ｹ謔ｶ・帝圦・ｦ邵ｺ・ｿ郢ｧ繝ｻ
    auto status = managedModel_->status.load();
    if (status == ManagedModel::LoadingStatus::Loaded && managedModel_->cpuModel) {
        InitializeResources();
    }
}

void AnimationModel::InitializeResources() {
    if (!managedModel_ || !managedModel_->cpuModel) {
        return;
    }

    // 4. 陞溽判驪､髯ｦ謔溘・郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ・ｮ騾墓ｻ薙・邵ｺ・ｨ郢晄ｧｭ繝｣郢昴・(陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･陷茨ｽｱ隴幄・逡・
    assert(engine_->GetDrawManager() && "DrawManager is not set.");
    transformationBuffer_.Initialize(engine_->GetDrawManager()->GetDxCommon());

    // 陷ｷ繝ｻﾎ鍋ｹ昴・縺咏ｹ晢ｽ･騾包ｽｨ郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ・ｮ騾墓ｻ薙・
    meshResources_.clear();
    for (size_t i = 0; i < managedModel_->gpuMeshes.size(); ++i) {
        auto res = std::make_unique<Object3DResource>();
        
        res->SetExternalTransformationBuffer(&transformationBuffer_);
        
        const auto& gpuMesh = managedModel_->gpuMeshes[i];
        res->vertexBufferView_ = gpuMesh->vertexBufferView;
        res->indexBufferView_ = gpuMesh->indexBufferView;
        res->indexCount_ = gpuMesh->indexCount;
        
        res->CreateResource();

        // 陋ｻ譎・ｄ郢昴・縺醍ｹｧ・ｹ郢昶・ﾎ慕ｹ昜ｸ莞ｦ郢晏ｳｨﾎ晉ｹｧ蛛ｵ縺慕ｹ晄鱒繝ｻ
        const auto& gpuMaterial = (i < managedModel_->gpuMaterials.size()) ? managedModel_->gpuMaterials[i] : nullptr;
        if (gpuMaterial) {
            res->textureHandle_ = gpuMaterial->textureHandle;
        }

        meshResources_.push_back(std::move(res));
    }

    assert(engine_ && "AnimationModel::Initialize: AnimationManager is not set.");
    animation_ = engine_->GetAnimationManager()->LoadAnimationFile(filename_);

    // Skeleton邵ｺ・ｮ騾墓ｻ薙・
    if (managedModel_ && managedModel_->cpuModel) {
        skeleton_ = AnimationManager::CreateSkeleton(managedModel_->cpuModel->rootNode);

        if (!managedModel_->cpuModel->skinClusterData.empty()) {
            skinCluster_ = engine_->GetAnimationManager()->CreateSkinCluster(skeleton_, *managedModel_->cpuModel);
        }

        jointSpheres_ = std::make_unique<SphereRegion>();
        jointSpheres_->Initialize(camera_, "resources/whiteTexture.png", 16);

        for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
            Transform tf{};
            tf.scale = { 0.01f, 0.01f, 0.01f };
            jointSpheres_->AddInstance(tf);
        }

        boneLines_ = std::make_unique<Line3DRegion>();
        boneLines_->Initialize(camera_);
    }

    animationTime_ = 0.0f;
    Update();
}

// 隴厄ｽｴ隴・ｽｰ
void AnimationModel::Update() {

    if (!managedModel_ || !camera_) return;

    // 鬮ｱ讒ｫ驟碑ｭ帶ｺ佩溽ｹ晢ｽｼ郢晏ｳｨ窶ｲ驍ｨ繧・ｽ冗ｸｺ・｣邵ｺ・ｦ邵ｺ繝ｻ・檎ｸｺ・ｰ隶堤距・ｯ蟲ｨ笘・ｹｧ繝ｻ(鬩輔・・ｻ・ｶ陋ｻ譎・ｄ陋ｹ繝ｻ
    if (managedModel_->status.load() == ManagedModel::LoadingStatus::Loaded && meshResources_.empty()) {
        InitializeResources();
    }

    // 邵ｺ・ｾ邵ｺ・ｰ雋・摩・咏ｸｺ・ｧ邵ｺ髦ｪ窶ｻ邵ｺ繝ｻ竊醍ｸｺ繝ｻ・ｰ・ｴ陷ｷ蛹ｻ繝ｻ郢ｧ・ｹ郢ｧ・ｭ郢昴・繝ｻ
    if (meshResources_.empty()) return;

    worldMatrix_ = Math::MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    UpdateAnimation();

    // 郢ｧ・ｹ郢ｧ・ｭ郢昜ｹ斟ｦ郢ｧ・ｰ郢晢ｽ｢郢昴・ﾎ晉ｸｺ荵昴Π郢晢ｽｼ郢晏ｳｨ縺・ｹ昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ郢晢ｽ｢郢昴・ﾎ晉ｸｺ荵昴定怎・ｦ騾・・・定崕繝ｻ・ｲ繝ｻ
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        // --- 郢ｧ・ｹ郢ｧ・ｭ郢昜ｹ斟ｦ郢ｧ・ｰ郢晢ｽ｢郢昴・ﾎ晉ｸｺ・ｮ隴厄ｽｴ隴・ｽｰ ---
        transformationMatrix_.world = worldMatrix_;
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));
        
        // SyncBeforeDraw() 邵ｺ・ｧ陷ｷ譴ｧ謔・ｸｺ蜷ｶ・狗ｸｺ貅假ｽ∫ｸｲ竏夲ｼ・ｸｺ阮吶堤ｸｺ・ｯ髫ｪ閧ｲ・ｮ蜉ｱ繝ｻ邵ｺ・ｿ髯ｦ蠕娯鴬
    } else {
        // --- 郢晏ｼｱ繝ｻ郢晏ｳｨ縺・ｹ昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ郢晢ｽ｢郢昴・ﾎ晉ｸｺ・ｮ隴厄ｽｴ隴・ｽｰ ---
        // 郢ｧ・ｪ郢晄じ縺夂ｹｧ・ｧ郢ｧ・ｯ郢昜ｺ･繝ｻ闖ｴ阮吶・郢晢ｽｯ郢晢ｽｼ郢晢ｽｫ郢晁歓・｡謔溘・郢ｧ螳夲ｽｨ閧ｲ・ｮ繝ｻ
        transformationMatrix_.world = localMatrix_ * worldMatrix_;

        // 雎墓・・ｷ螢ｼ・､逕ｻ驪､騾包ｽｨ邵ｺ・ｮ鬨ｾ繝ｻ・ｻ・｢驗ゑｽｮ髯ｦ謔溘・
        Matrix4x4 worldForNormal = transformationMatrix_.world;
        worldForNormal.m[3][0] = 0.0f; worldForNormal.m[3][1] = 0.0f;
        worldForNormal.m[3][2] = 0.0f; worldForNormal.m[3][3] = 1.0f;
        transformationMatrix_.WorldInverseTranspose = Math::Transpose(Math::Inverse(worldForNormal));

        // 髫ｪ閧ｲ・ｮ蜉ｱ・邵ｺ貅ｯ・｡謔溘・郢ｧ蛛ｵ繝ｻ郢昴・繝ｻ雋ょ現竏ｩ邵ｺ・ｮ郢晢ｽｪ郢ｧ・ｽ郢晢ｽｼ郢ｧ・ｹ邵ｺ・ｫ郢ｧ・ｳ郢晄鱒繝ｻ
        // SyncBeforeDraw() 邵ｺ・ｧ陷ｷ譴ｧ謔・ｸｺ蜷ｶ・狗ｸｺ貅假ｽ∫ｸｲ竏夲ｼ・ｸｺ阮吶堤ｸｺ・ｯ髫ｪ閧ｲ・ｮ蜉ｱ繝ｻ邵ｺ・ｿ髯ｦ蠕娯鴬
    }


    // 陷ｷﾐ冩int邵ｺ・ｮ闖ｴ蜥ｲ・ｽ・ｮ郢ｧ諡・hereRegion邵ｺ・ｫ陷ｿ閧ｴ荳・
    boneLines_->ClearInstances();
    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        // Joint邵ｺ・ｮSkeleton驕ｨ・ｺ鬮｢阮吶堤ｸｺ・ｮ髯ｦ謔溘・郢ｧ雋槫徐陟輔・
        const Matrix4x4& jointMat = skeleton_.joints[i].skeletonSpaceMatrix;

        // Joint邵ｺ・ｮ郢晢ｽｯ郢晢ｽｼ郢晢ｽｫ郢晉甥・ｺ・ｧ隶薙・= Joint(Skeleton驕ｨ・ｺ鬮｢繝ｻ * 郢晢ｽ｢郢昴・ﾎ晉ｸｺ・ｮWorld髯ｦ謔溘・
        Matrix4x4 jointWorldMat = jointMat * worldMatrix_;

        // 髯ｦ謔溘・邵ｺ荵晢ｽ芽抄蜥ｲ・ｽ・ｮ繝ｻ繝ｻranslate繝ｻ蟲ｨ笆｡邵ｺ莉｣・定ｬ夲ｽｽ陷・ｽｺ
        Vector3 jointPosition = {
            jointWorldMat.m[3][0],
            jointWorldMat.m[3][1],
            jointWorldMat.m[3][2]
        };

        // SphereRegion邵ｺ・ｮi騾｡・ｪ騾ｶ・ｮ邵ｺ・ｮ郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ邵ｺ・ｮ闖ｴ蜥ｲ・ｽ・ｮ郢ｧ蜻亥ｳｩ隴・ｽｰ
        Transform tf;
        tf.scale = { 0.01f, 0.01f, 0.01f };
        tf.rotate = { 0.0f, 0.0f, 0.0f }; // 騾・・・ｽ阮吮・邵ｺ・ｮ邵ｺ・ｧ陜玲ｫ・ｽｻ・｢邵ｺ・ｯ霎滂ｽ｡髫墓じ縲丹K
        tf.translate = jointPosition;

        jointSpheres_->UpdateInstance(static_cast<uint32_t>(i), tf);

        // 髫包ｽｪ郢ｧ・ｸ郢晢ｽｧ郢ｧ・､郢晢ｽｳ郢晏現窶ｲ邵ｺ繧・ｽ檎ｸｺ・ｰ邵ｲ竏ｬ・ｦ・ｪ邵ｺ荵晢ｽ蛾明・ｪ陋ｻ繝ｻ竏育ｸｺ・ｮ驍ｱ螟ｲ・ｼ蛹ｻ繝ｻ郢晢ｽｼ郢晢ｽｳ繝ｻ蟲ｨ・定ｬ蜀怜愛
        if (skeleton_.joints[i].parent) {
            const int32_t parentIndex = *skeleton_.joints[i].parent;
            const Matrix4x4& parentMat = skeleton_.joints[parentIndex].skeletonSpaceMatrix;
            Matrix4x4 parentWorldMat = parentMat * worldMatrix_;
            Vector3 parentPosition = {
                parentWorldMat.m[3][0],
                parentWorldMat.m[3][1],
                parentWorldMat.m[3][2]
            };
            boneLines_->AddInstance(parentPosition, jointPosition, { 1.0f, 1.0f, 0.0f, 1.0f });
        }
    }

    // 郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ隲繝ｻ・ｰ・ｱ郢ｧ遶ｪPU邵ｺ・ｸ髴・ｽ｢鬨ｾ繝ｻ
    UpdateMaterials();

    MarkAsDirty();
    isDirty_ = false;
    lastViewMatrix_ = camera_->GetViewMatrix();
    lastProjectionMatrix_ = camera_->GetPerspectiveFovMatrix();
    // 郢ｧ・ｹ郢ｧ・ｭ郢昜ｹ斟ｦ郢ｧ・ｰ郢晢ｽ｢郢昴・ﾎ晉ｸｺ・ｮ陜｣・ｴ陷ｷ蛹ｻ繝ｻCompute Shader邵ｺ・ｮ陞ｳ貅ｯ・｡蠕鯉ｽ定滋閧ｲ・ｴ繝ｻ笘・ｹｧ繝ｻ
    if (!managedModel_->cpuModel->skinClusterData.empty() && engine_ && engine_->GetDrawManager()) {
        engine_->GetDrawManager()->RegisterComputeTask(this);
    }
}

void AnimationModel::SyncBeforeDraw() {
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    
    // 陞溽判驪､髯ｦ謔溘・邵ｺ・ｮ隴厄ｽｴ隴・ｽｰ (陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･邵ｺ・ｧ陷茨ｽｱ隴帙・
    if (isDirtyBuffer_[frameIndex]) {
        transformationBuffer_.Update(transformationMatrix_, frameIndex);
        isDirtyBuffer_[frameIndex] = false;
    }
    
    // 陷ｷ繝ｻﾎ鍋ｹ昴・縺咏ｹ晢ｽ･邵ｺ・ｮ郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ驕ｲ蟲ｨ繝ｻ隴厄ｽｴ隴・ｽｰ
    for (auto& res : meshResources_) {
        res->SyncBeforeDraw();
    }
    
    // --- SkinCluster 邵ｺ・ｮ郢晄ｧｭﾎ晉ｹ昶・繝ｰ郢昴・繝ｵ郢ｧ・｡陷ｷ譴ｧ謔・・蛹ｻ繝ｻ郢晢ｽｼ郢ｧ・ｺ闕ｳ・ｭ邵ｺ・ｮ隰厄ｽｯ陷榊供・ｯ・ｾ驕ｲ蜴・ｽｼ繝ｻ---
    if (managedModel_ && managedModel_->cpuModel && !managedModel_->cpuModel->skinClusterData.empty()) {
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
    }
}

void AnimationModel::DispatchCompute() {
    if (!managedModel_ || !managedModel_->cpuModel || managedModel_->cpuModel->skinClusterData.empty() || !camera_) return;

    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        boundingSphere.radius = managedModel_->cpuModel->boundingSphere.radius * maxScale * 1.5f;
        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            return; // 髫募､懷ｹ陷ｿ・ｰ郢ｧ・ｫ郢晢ｽｪ郢晢ｽｳ郢ｧ・ｰ邵ｺ霈費ｽ檎ｸｺ・ｦ邵ｺ繝ｻ・玖撻・ｴ陷ｷ蛹ｻ繝ｻCompute郢ｧ繧・○郢ｧ・ｭ郢昴・繝ｻ
        }
    }

    engine_->GetDrawManager()->DispatchSkinning(skinCluster_, managedModel_.get(), skinCluster_.mappedSkinningInformation->numVertices);
    uint32_t f = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    engine_->GetDrawManager()->ExecuteUAVBarrier(skinCluster_.skinnedVertexResource[f].Get());
}

// 隰蜀怜愛
void AnimationModel::Draw() {
    if (!managedModel_ || !camera_ || meshResources_.empty()) return;

    // 髫募､懷ｹ陷ｿ・ｰ郢ｧ・ｫ郢晢ｽｪ郢晢ｽｳ郢ｧ・ｰ
    if (isCullingEnabled_) {
        float maxScale = (std::max)({ transform_.scale.x, transform_.scale.y, transform_.scale.z });
        
        Sphere boundingSphere;
        boundingSphere.center = transform_.translate;
        // 郢ｧ・｢郢昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ邵ｺ・ｫ郢ｧ蛹ｻ・玖弱・窶ｲ郢ｧ鄙ｫ・帝蔓繝ｻ繝ｻ邵ｺ蜉ｱﾂ竏墅皮ｹ昴・ﾎ晁・・髦憺・・繝ｻ1.5陋滄亂繝ｻ郢晄ｧｭ繝ｻ郢ｧ・ｸ郢晢ｽｳ郢ｧ螳夲ｽｨ・ｭ陞ｳ繝ｻ
        boundingSphere.radius = managedModel_->cpuModel->boundingSphere.radius * maxScale * 1.5f;

        if (!Collision::IsCollision(camera_->GetFrustum(), boundingSphere)) {
            return; // 隰蜀怜愛郢ｧ・ｹ郢ｧ・ｭ郢昴・繝ｻ
        }
    }

    // 郢ｧ・ｫ郢晢ｽ｡郢晢ｽｩ邵ｺ・ｮ髯ｦ謔溘・邵ｺ謔滂ｽ､逕ｻ蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ邵ｲ竏壹′郢晄じ縺夂ｹｧ・ｧ郢ｧ・ｯ郢晞メ繝ｻ闖ｴ阮吮ｲ陞溽判蟲ｩ邵ｺ霈費ｽ檎ｸｺ貅伉ｰ郢昶・縺臥ｹ昴・縺・
    bool cameraChanged = (std::memcmp(&lastViewMatrix_, &camera_->GetViewMatrix(), sizeof(Matrix4x4)) != 0 ||
                          std::memcmp(&lastProjectionMatrix_, &camera_->GetPerspectiveFovMatrix(), sizeof(Matrix4x4)) != 0);

    if (isDirty_ || cameraChanged) {
        Update();
    }
    
    // --- 邵ｲ蜊・ｿ・ｽ陷会｣ｰ邵ｲ隨ｬ邱帝包ｽｻ騾ｶ・ｴ陷鷹亂繝ｻ郢晁・繝｣郢晁ｼ斐＜陷ｷ譴ｧ謔・---
    SyncBeforeDraw();

    // --- 髴托ｽｽ陷会｣ｰ繝ｻ螟撰ｽｪ・ｨ隴ｬ・ｼ繝ｻ閧ｲ蟶･闖ｴ阮吶・鬮ｮ繝ｻ邊九・蟲ｨ・定叉ﾂ隲｡・ｬ隰蜀怜愛 ---
    if (jointSpheres_ && !skeleton_.joints.empty()) {
        engine_->ApplyRegionPSO();
        jointSpheres_->Draw();
    }

    // --- 髴托ｽｽ陷会｣ｰ繝ｻ螢ｹ繝ｻ郢晢ｽｼ郢晢ｽｳ繝ｻ閧ｲ・ｷ螟ｲ・ｼ蟲ｨ・定叉ﾂ隲｡・ｬ隰蜀怜愛 ---
    if (boneLines_ && !skeleton_.joints.empty()) {
        engine_->ApplyLineInstancedPSO();
        boneLines_->Draw();
    }

    // 2. 郢ｧ・ｰ郢晢ｽｩ郢晁ｼ斐≦郢昴・縺醍ｹｧ・ｹPSO邵ｺ・ｮ鬩包ｽｩ騾包ｽｨ
    engine_->ApplyPSO();

    // 3. 陷茨ｽｨ郢晢ｽ｡郢昴・縺咏ｹ晢ｽ･郢ｧ蛛ｵﾎ晉ｹ晢ｽｼ郢晏干・邵ｺ・ｦ隰蜀怜愛
    uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
    for (size_t i = 0; i < meshResources_.size(); ++i) {
        auto& res = meshResources_[i];

        // 郢ｧ・ｹ郢ｧ・ｭ郢昜ｹ斟ｦ郢ｧ・ｰ闕ｳ・ｭ邵ｺ・ｪ郢ｧ繝ｻVBV 郢ｧ雋橸ｽｷ・ｮ邵ｺ邇ｲ蟠帷ｸｺ蛹ｻ窶ｻ隰蜀怜愛
        if (!managedModel_->cpuModel->skinClusterData.empty()) {
            engine_->GetDrawManager()->DrawStandard3D(res.get(), &skinCluster_.skinnedVertexBufferView[frameIndex]);
        } else {
            engine_->GetDrawManager()->DrawStandard3D(res.get());
        }
    }
}

// 郢昴・繝ｰ郢昴・縺・
void AnimationModel::Debug([[maybe_unused]] const char* objName) {
#if defined USE_IMGUI
    std::string name = std::string("AnimationModel: ") + objName;
    ImGui::Begin(name.c_str());
    if (engine_) {
        auto* ui_ = engine_->GetDebugUI();
        ui_->DebugTransform(transform_);
        ui_->DebugAnimationControl(animation_, animationTime_);
        ImGui::Checkbox("Frustum Culling", &isCullingEnabled_);

        if (ImGui::Button("Reset Animation Time")) {
            animationTime_ = 0.0f;
        }

        ImGui::ColorEdit4("Color", &color_.x); // 郢ｧ・､郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｿ郢晢ｽｳ郢ｧ・ｹ郢ｧ・ｫ郢晢ｽｩ郢晢ｽｼ郢ｧ蝣､・ｷ・ｨ鬮ｮ繝ｻ
        ui_->DebugMaterialOverrides(&environmentCoefficient_, &lightingModeOverride_, &useClampSamplerOverride_, &enableLightingOverride_, "##AmOverrides");

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
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
#endif
}

void AnimationModel::UpdateMaterials() {
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
        
        // (郢晄ｧｭ繝ｦ郢晢ｽｪ郢ｧ・｢郢晢ｽｫ郢晁・繝｣郢晁ｼ斐＜邵ｺ・ｸ邵ｺ・ｮ髴・ｽ｢鬨ｾ竏壹・ SyncBeforeDraw() 邵ｺ・ｧ髯ｦ蠕鯉ｽ冗ｹｧ蠕鯉ｽ狗ｸｺ貅假ｽ∫ｸｲ竏夲ｼ・ｸｺ阮吶堤ｸｺ・ｯ SyncMaterialData 邵ｺ・ｯ陷ｻ・ｼ邵ｺ・ｰ邵ｺ・ｪ邵ｺ繝ｻ
        res->MarkAsDirty();
    }
}


void AnimationModel::UpdateAnimation() {

    // 郢ｧ・｢郢昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ邵ｺ・ｮ陷・ｽｦ騾・・

    animationTime_ += 1.0f / 60.0f; // 隴弱ｇ邯ｾ郢ｧ蟶敖・ｲ郢ｧ竏夲ｽ狗ｸｲ繝ｻ/60邵ｺ・ｧ陜暦ｽｺ陞ｳ螢ｹ・邵ｺ・ｦ邵ｺ繧・ｽ狗ｸｺ蠕個竏ｬ・ｨ蝓滂ｽｸ・ｬ邵ｺ蜉ｱ笳・ｭ弱ｋ菫｣郢ｧ蜑・ｽｽ・ｿ邵ｺ・｣邵ｺ・ｦ陷ｿ・ｯ陞溷ｳｨ繝ｵ郢晢ｽｬ郢晢ｽｼ郢晢｣ｰ郢ｧ雋橸ｽｯ・ｾ陟｢諛奇ｼ邵ｺ貅倪括邵ｺ繝ｻ窶ｲ隴帛ｸ吮穐邵ｺ蜉ｱ・樒ｸｲ繝ｻ
    animationTime_ = std::fmod(animationTime_, animation_.duration); // 隴崢陟募ｾ娯穐邵ｺ・ｧ髫ｪﾂ邵ｺ・｣邵ｺ貅假ｽ芽ｭ崢陋ｻ譏ｴﾂｰ郢ｧ蟲ｨﾎ懃ｹ晄鱒繝ｻ郢昜ｺ･繝ｻ騾墓ｺ伉繧・懃ｹ晄鱒繝ｻ郢晏現・邵ｺ・ｪ邵ｺ荳岩ｻ郢ｧ繧・肩邵ｺ・ｫ邵ｺ繝ｻ・樒ｸｲ繝ｻ

    // 郢ｧ・ｹ郢ｧ・ｭ郢昜ｹ斟ｦ郢ｧ・ｰ郢ｧ・｢郢昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ邵ｺ・ｮ陜｣・ｴ陷ｷ繝ｻ
    if (!managedModel_->cpuModel->skinClusterData.empty()) {
        // 1. 陷茨ｽｨJoint邵ｺ・ｫ郢ｧ・｢郢昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ郢ｧ蟶昶・騾包ｽｨ
        AnimationManager::ApplyAnimation(skeleton_, animation_, animationTime_);

        // 2. 鬮ｫ荳ｻ・ｱ・､隶堤洸ﾂ・ｰ邵ｺ・ｮ髯ｦ謔溘・隴厄ｽｴ隴・ｽｰ
        AnimationManager::SkeletonUpdate(skeleton_);

        // 3. SkinCluster邵ｺ・ｮMatrixPalette郢ｧ蜻亥ｳｩ隴・ｽｰ
        uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
        AnimationManager::SkinClusterUpdate(skinCluster_, skeleton_, frameIndex);
    } else { // 郢晏ｼｱ繝ｻ郢晏ｳｨ縺・ｹ昜ｹ斟鍋ｹ晢ｽｼ郢ｧ・ｷ郢晢ｽｧ郢晢ｽｳ邵ｺ・ｮ陜｣・ｴ陷ｷ繝ｻ
        // rootNode邵ｺ・ｮAnimation郢ｧ雋槫徐陟輔・
        NodeAnimation& rootNodeAnimation = animation_.nodeAnimations[managedModel_->cpuModel->rootNode.name];
        // 隰悶・・ｮ螢ｽ蜃ｾ陋ｻ・ｻ邵ｺ・ｮ陋滂ｽ､郢ｧ雋槫徐陟輔・
        Vector3 translate = AnimationManager::CalculateValue(rootNodeAnimation.translate, animationTime_);
        Quaternion rotate = AnimationManager::CalculateValue(rootNodeAnimation.rotate, animationTime_);
        Vector3 scale = AnimationManager::CalculateValue(rootNodeAnimation.scale, animationTime_);
        localMatrix_ = Math::MakeAffineMatrix(scale, rotate, translate);
    }
}

const ObjMaterial* AnimationModel::GetMaterial(size_t meshIndex) const {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}

ObjMaterial* AnimationModel::GetMaterial(size_t meshIndex) {
    if (managedModel_ && managedModel_->cpuModel && meshIndex < managedModel_->cpuModel->meshes.size()) {
        return &managedModel_->cpuModel->meshes[meshIndex].material;
    }
    return nullptr;
}