#pragma once
#include "Framework/Component/Component.h"
#include <string>

class ReticleUIComponent : public Component {
public:
    ReticleUIComponent() = default;
    ~ReticleUIComponent() override = default;

    void Initialize() override;
    void Update() override;

    std::string GetComponentName() const override {
        return "ReticleUIComponent";
    }
};
