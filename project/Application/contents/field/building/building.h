#pragma once

#include "Irufemi.h"
#include <vector>
#include <memory>
#include <string>

#include "Renderer/LineInstanced/LineClass.h"
#include "Engine/Core/Math/Vector3.h"

class Camera;
class IrufemiEngine;
class ObjClass;

class Building {
public:
    Building();
    ~Building();

    void Initialize(Camera* camera, IrufemiEngine* engine);
    void Update();
    void Draw();
    void DrawImGui();

private:
    void LoadJson();
    void SaveJson();
    void Generate();

private:
    Camera* camera_ = nullptr;
    IrufemiEngine* engine_ = nullptr;

    std::vector<std::unique_ptr<ObjClass>> buildings_;

    // パラメータ
    struct Parameters {
        int count = 10;
        float minHeight = 10.0f;
        float maxHeight = 50.0f;
        float minScaleXZ = 5.0f;
        float maxScaleXZ = 15.0f;
        float fieldRange = 90.0f;
    } params_;

    const std::string kJsonFilePath = "resources/Json/building/parameters.json";

#ifdef USE_IMGUI
    std::unique_ptr<Line3DRegion> debugLines_;
    bool isDebugDraw_ = false;
#endif
};
