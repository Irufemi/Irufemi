#pragma once

#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Vector4.h"
#include "../Core/IRenderable.h"
#include <memory>
#include <string>
#include <vector>

class Object3DResource;
class IrufemiEngine;

class WeaponTrail : public IRenderable {
public:
    WeaponTrail();
    ~WeaponTrail();

    void Initialize(IrufemiEngine* engine, const std::string& texturePath, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    void Update();
    void SyncBeforeDraw() override;
    void Draw() override;

    void AddPoint(const Vector3& basePos, const Vector3& tipPos);
    void StopTrail();

private:
    struct TrailPoint {
        Vector3 basePos;
        Vector3 tipPos;
        int age;
    };
    std::vector<TrailPoint> points_;
    std::unique_ptr<Object3DResource> resource_ = nullptr;
    Vector4 baseColor_;
    std::string texturePath_;
    bool isStopped_ = true;

    IrufemiEngine* engine_ = nullptr;

    static constexpr int kMaxPoints = 64;
    static constexpr int kMaxLifeTime = 30;
};
