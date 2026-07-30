#include "BoneAttachmentComponent.h"
#include "Framework/GameObject.h"
#include "Framework/BaseScene.h"
#include "Framework/Component/TransformComponent.h"
#include "Framework/Component/Renderer/SkinnedMeshRendererComponent.h"
#include "Resource/Model/Data/SkeletonPose.h"
#include "Resource/Model/Data/SkeletonData.h"
#include "Engine/Core/Math/MathFunction.h"

BoneAttachmentComponent::BoneAttachmentComponent() = default;
BoneAttachmentComponent::~BoneAttachmentComponent() = default;

void BoneAttachmentComponent::Initialize() {}

void BoneAttachmentComponent::Update() {
    if (targetName_.empty() || targetBoneName_.empty()) return;

    auto gameObject = GetGameObject();
    if (!gameObject) return;
    
    auto transform = gameObject->GetComponent<TransformComponent>();
    if (!transform) return;

    auto scene = gameObject->GetScene();
    if (!scene) return;

    // ターゲットとなるGameObjectを探す
    auto targetObj = scene->FindGameObject(targetName_);
    if (!targetObj) return;

    // ターゲットのSkinnedMeshRendererComponentを探す
    auto renderer = targetObj->GetComponent<SkinnedMeshRendererComponent>();
    if (renderer && renderer->GetRawObject()) {
        const SkeletonPose* pose = renderer->GetRawObject()->GetInternalSkeletonPose();
        if (pose && pose->data) {
            auto it = pose->data->jointMap.find(targetBoneName_);
            if (it != pose->data->jointMap.end()) {
                int index = it->second;
                // ローカルのボーン行列を取得
                Irufemi::Matrix4x4 localMat = pose->jointPoses[index].skeletonSpaceMatrix;
                
                // 親のワールド行列と掛けてボーンの最終ワールド行列を算出
                auto targetTransform = targetObj->GetComponent<TransformComponent>();
                Irufemi::Matrix4x4 boneWorldMat = localMat;
                if (targetTransform) {
                    boneWorldMat = localMat * targetTransform->GetWorldMatrix();
                }

                // 自身のTransformComponentに適用
                // 位置と回転を同期
                Irufemi::Vector3 boneWorldPos = { boneWorldMat.m[3][0], boneWorldMat.m[3][1], boneWorldMat.m[3][2] };
                transform->SetPosition(boneWorldPos);
                
                // 行列から回転成分を抽出してEuler角に変換
                Irufemi::Vector3 rot = Irufemi::Math::ExtractEulerFromMatrix(boneWorldMat);
                transform->SetRotation(rot);
            }
        }
    }
}

void BoneAttachmentComponent::OnRegisterProperties() {
    RegisterProperty("Target Name", &targetName_);
    RegisterProperty("Target Bone Name", &targetBoneName_);
}

nlohmann::json BoneAttachmentComponent::Serialize() {
    nlohmann::json j = nlohmann::json::object();
    j["Target Name"] = targetName_;
    j["Target Bone Name"] = targetBoneName_;
    return j;
}

void BoneAttachmentComponent::Deserialize(const nlohmann::json& j) {
    if (j.contains("Target Name")) targetName_ = j["Target Name"];
    if (j.contains("Target Bone Name")) targetBoneName_ = j["Target Bone Name"];
}
