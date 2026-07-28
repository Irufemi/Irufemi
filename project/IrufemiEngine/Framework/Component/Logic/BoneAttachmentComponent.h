#pragma once
#include "Framework/Component/Component.h"
#include <string>

class BoneAttachmentComponent : public Component {
public:
    BoneAttachmentComponent();
    ~BoneAttachmentComponent() override;

    void Initialize() override;
    void Update() override;
    void OnRegisterProperties() override;

    std::string GetComponentName() const override { return "BoneAttachmentComponent"; }

    nlohmann::json Serialize() override;
    void Deserialize(const nlohmann::json& j) override;

    void SetTargetName(const std::string& targetName) { targetName_ = targetName; }
    void SetTargetBoneName(const std::string& boneName) { targetBoneName_ = boneName; }

private:
    std::string targetName_ = "";
    std::string targetBoneName_ = "";
};
