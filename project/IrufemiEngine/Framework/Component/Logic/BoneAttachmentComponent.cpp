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
    
    auto transform = GetTransform();
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

                // 自身のTransformComponentに適用するため、自身の親の逆行列を掛けてローカル行列に変換する
                Irufemi::Matrix4x4 finalLocalMat = boneWorldMat;
                if (auto myParent = gameObject->GetParent()) {
                    if (auto parentT = myParent->GetComponent<TransformComponent>()) {
                        finalLocalMat = Irufemi::Math::Multiply(boneWorldMat, Irufemi::Math::Inverse(parentT->GetWorldMatrix()));
                    }
                }

                // 位置と回転を同期
                Irufemi::Vector3 boneLocalPos = { finalLocalMat.m[3][0], finalLocalMat.m[3][1], finalLocalMat.m[3][2] };
                transform->SetPosition(boneLocalPos);
                
                // 行列から回転成分を抽出してEuler角に変換
                Irufemi::Vector3 rot = Irufemi::Math::ExtractEulerFromMatrix(finalLocalMat);
                transform->SetRotation(rot);

                // 更新を即座に反映させる（次のコンポーネントが描画などに使うため）
                transform->MarkWorldDirty();
                transform->ComputeMatrix(true);
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
