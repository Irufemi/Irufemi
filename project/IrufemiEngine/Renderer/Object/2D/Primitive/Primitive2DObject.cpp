#include "Primitive2DObject.h"
#include "Engine/Graphics/Camera/CameraManager.h"
#include "Engine/IrufemiEngine.h"
#include "Engine/Manager/DebugUI.h"
#include "Engine/Manager/DrawManager.h"
#include "Resource/Texture/TextureManager.h"

#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui/imgui.h"
#endif

#include <windows.h>
#include <string>

// --- 外部依存マネージャの静的初期化 ---
TextureManager* Primitive2DObject::textureManager_ = nullptr;
DrawManager* Primitive2DObject::drawManager_ = nullptr;
DebugUI* Primitive2DObject::ui_ = nullptr;
IrufemiEngine* Primitive2DObject::engine_ = nullptr;

// --- Initialize / Update / Draw ---

void Primitive2DObject::Initialize(Primitive2DType type, const std::string& textureName) {
    resource_ = std::make_unique<Object2DResource>();
    type_ = type;
    isMeshDirty_ = true;

    // デフォルトマテリアル設定
    resource_->GetMaterialData()->color = {1.0f, 1.0f, 1.0f, 1.0f};
    resource_->GetMaterialData()->enableLighting = false;

    SetTexture(textureName);

    Update();
}

void Primitive2DObject::Update() {
    if (isMeshDirty_) {
        RebuildMesh();
        isMeshDirty_ = false;
    }

    if (resource_ && engine_) {
        if (CameraManager* camM = engine_->GetCameraManager()) {
            if (Camera* activeCam = camM->GetActiveCamera()) {
                resource_->UpdateTransform(*activeCam);
            }
        }
    }
    
    // 描画がうまくいかない場合のデバッグ用
    static int frameCount = 0;
    if (frameCount++ % 60 == 0) {
        char buf[512];
        sprintf_s(buf, "[Primitive2D] IndexCount: %d, VertData: %p, MatIndex: %u\n", 
            resource_->indexCount_, (void*)resource_->vertexData_, resource_->materialCbIndex_);
        OutputDebugStringA(buf);
    }
}

void Primitive2DObject::SyncBeforeDraw() {
    if (resource_) {
        resource_->SyncBeforeDraw();
    }
}

void Primitive2DObject::Draw() {
    if (!resource_ || !drawManager_)
        return;

    SyncBeforeDraw();

    if (isTopMost_) {
        drawManager_->SubmitTopMostSprite(resource_.get());
    } else {
        drawManager_->SubmitSprite(resource_.get()); // 通常2D描画
    }
}

// --- Debug ---

void Primitive2DObject::Debug(const char* label) {
#ifdef USE_IMGUI
    if (!ui_)
        return;

    ImGui::Begin(label);

    const char* shapeNames[] = {"Rect", "Triangle", "Circle", "Ring", "Line"};
    int currentShape = static_cast<int>(type_);
    if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames))) {
        SetShape(static_cast<Primitive2DType>(currentShape));
    }

    // --- Transform & Size ---
    if (ImGui::CollapsingHeader("Transform & Layout", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::DragFloat2("Size", &size_.x, 1.0f))
            SetSize(size_);
        if (ImGui::DragFloat2("Pivot", &pivot_.x, 0.01f, 0.0f, 1.0f))
            SetPivot(pivot_);

        Vector3 pos = GetPosition();
        if (ImGui::DragFloat3("Position", &pos.x, 1.0f))
            SetPosition(pos);

        Vector3 rot = GetRotation();
        if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
            resource_->transform_.rotate = rot;
        }

        Vector3 scale = GetScale();
        if (ImGui::DragFloat3("Scale", &scale.x, 0.01f))
            SetScale(scale);
    }

    // --- Shape Params ---
    if (type_ == Primitive2DType::Ring || type_ == Primitive2DType::Line) {
        if (ImGui::DragFloat("Thickness", &thickness_, 1.0f, 1.0f, 1000.0f))
            SetThickness(thickness_);
    }
    if (type_ == Primitive2DType::Circle || type_ == Primitive2DType::Ring) {
        int sub = static_cast<int>(subdivision_);
        if (ImGui::DragInt("Subdivision", &sub, 1, 3, 128)) {
            subdivision_ = static_cast<uint32_t>(sub);
            isMeshDirty_ = true;
        }
    }

    // --- Material ---
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit4("Color", &resource_->GetMaterialData()->color.x);

        if (textureManager_) {
            ui_->DebugTexture(resource_.get(), selectedTextureIndex_);
            auto names = textureManager_->GetTextureNamesForDebug();
            if (selectedTextureIndex_ >= 0 && selectedTextureIndex_ < static_cast<int>(names.size())) {
                SetTexture(names[selectedTextureIndex_]);
            }
        }
    }

    ImGui::Checkbox("Top Most", &isTopMost_);

    ImGui::End();
#endif
}

// --- Setters ---

void Primitive2DObject::SetShape(Primitive2DType type) {
    if (type_ != type) {
        type_ = type;
        isMeshDirty_ = true;
    }
}

void Primitive2DObject::SetSize(const Vector2& size) {
    size_ = size;
    isMeshDirty_ = true;
}

void Primitive2DObject::SetPivot(const Vector2& pivot) {
    pivot_ = pivot;
    isMeshDirty_ = true;
}

void Primitive2DObject::SetPosition(const Vector3& position) {
    if (resource_)
        resource_->transform_.translate = position;
}

void Primitive2DObject::SetRotationZ(float rad) {
    if (resource_)
        resource_->transform_.rotate.z = rad;
}

void Primitive2DObject::SetScale(const Vector3& scale) {
    if (resource_)
        resource_->transform_.scale = scale;
}

void Primitive2DObject::SetColor(const Vector4& color) {
    if (resource_)
        resource_->GetMaterialData()->color = color;
}

void Primitive2DObject::SetTexture(const std::string& textureName) {
    if (textureManager_) {
        ResourceHandle handle = textureManager_->LoadTexture(textureName);
        if (resource_) {
            resource_->textureHandle_ = handle;
            resource_->GetMaterialData()->hasTexture = true;
        }

        auto names = textureManager_->GetTextureNamesForDebug();
        auto it = std::find(names.begin(), names.end(), textureName);
        if (it != names.end()) {
            selectedTextureIndex_ = static_cast<int>(std::distance(names.begin(), it));
        }
    }
}

void Primitive2DObject::SetThickness(float thickness) {
    thickness_ = thickness;
    if (type_ == Primitive2DType::Ring || type_ == Primitive2DType::Line) {
        isMeshDirty_ = true;
    }
}

// --- Mesh Builders ---

void Primitive2DObject::RebuildMesh() {
    if (!resource_)
        return;

    resource_->vertexDataList_.clear();
    resource_->indexDataList_.clear();

    switch (type_) {
    case Primitive2DType::Rect:
        BuildRect();
        break;
    case Primitive2DType::Triangle:
        BuildTriangle();
        break;
    case Primitive2DType::Circle:
        BuildCircle(subdivision_);
        break;
    case Primitive2DType::Ring:
        BuildRing(subdivision_);
        break;
    case Primitive2DType::Line:
        BuildLine();
        break;
    }

    // リソース再生成（頂点・インデックス）
    resource_->CreateResource();
    resource_->Map();

    // データのコピー
    if (resource_->vertexData_) {
        std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);
    }
    if (resource_->indexData_) {
        std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);
    }
}

void Primitive2DObject::BuildRect() {
    // pivotを基準にしたローカルのAABBを計算
    // 左上: (0,0) で pivot=(0,0) の場合、左=0, 右=size.x, 上=0, 下=size.y
    // 中心: pivot=(0.5, 0.5) の場合、左=-size.x/2, 右=size.x/2, 上=-size.y/2, 下=size.y/2
    float left = -size_.x * pivot_.x;
    float right = size_.x * (1.0f - pivot_.x);
    float top = -size_.y * pivot_.y;
    float bottom = size_.y * (1.0f - pivot_.y);

    resource_->vertexDataList_ = {
        {{left, top, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},    // 左上
        {{right, top, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},   // 右上
        {{left, bottom, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
        {{right, bottom, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}} // 右下
    };

    resource_->indexDataList_ = {0, 1, 2, 2, 1, 3};
}

void Primitive2DObject::BuildTriangle() {
    float left = -size_.x * pivot_.x;
    float right = size_.x * (1.0f - pivot_.x);
    float top = -size_.y * pivot_.y;
    float bottom = size_.y * (1.0f - pivot_.y);
    float centerX = left + size_.x * 0.5f;

    resource_->vertexDataList_ = {
        {{centerX, top, 0.0f, 1.0f}, {0.5f, 0.0f}, {0.0f, 0.0f, -1.0f}},  // 上
        {{right, bottom, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 右下
        {{left, bottom, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}   // 左下
    };

    resource_->indexDataList_ = {0, 1, 2};
}

void Primitive2DObject::BuildCircle(uint32_t subdiv) {
    if (subdiv < 3)
        subdiv = 3;

    // サイズはXとYで楕円をサポート
    float radiusX = size_.x * 0.5f;
    float radiusY = size_.y * 0.5f;

    // ピボットからのオフセット計算（ローカルでの中心位置）
    // pivot=(0.5, 0.5) のとき offsetX=0, offsetY=0
    float offsetX = (0.5f - pivot_.x) * size_.x;
    float offsetY = (0.5f - pivot_.y) * size_.y;

    // 中心点
    resource_->vertexDataList_.push_back({{offsetX, offsetY, 0.0f, 1.0f}, {0.5f, 0.5f}, {0.0f, 0.0f, -1.0f}});

    float pi = 3.141592654f;
    for (uint32_t i = 0; i <= subdiv; ++i) {
        float rate = static_cast<float>(i) / subdiv;
        float angle = rate * pi * 2.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        float vx = offsetX + cosA * radiusX;
        float vy = offsetY + sinA * radiusY; // y軸下向き・上向きはエンジン規約に依存するが、ここでは標準的なsinを使用
        float u = cosA * 0.5f + 0.5f;
        float v = sinA * 0.5f + 0.5f;

        resource_->vertexDataList_.push_back({{vx, vy, 0.0f, 1.0f}, {u, v}, {0.0f, 0.0f, -1.0f}});
    }

    for (uint32_t i = 0; i < subdiv; ++i) {
        resource_->indexDataList_.push_back(0); // 中心
        resource_->indexDataList_.push_back(i + 1);
        resource_->indexDataList_.push_back(i + 2);
    }
}

void Primitive2DObject::BuildRing(uint32_t subdiv) {
    if (subdiv < 3)
        subdiv = 3;

    float outerRadiusX = size_.x * 0.5f;
    float outerRadiusY = size_.y * 0.5f;

    // 内径は太さ(thickness_)分だけ小さい（ピクセル単位を想定）
    float innerRadiusX = (std::max)(0.0f, outerRadiusX - thickness_);
    float innerRadiusY = (std::max)(0.0f, outerRadiusY - thickness_);

    float offsetX = (0.5f - pivot_.x) * size_.x;
    float offsetY = (0.5f - pivot_.y) * size_.y;

    float pi = 3.141592654f;
    for (uint32_t i = 0; i <= subdiv; ++i) {
        float rate = static_cast<float>(i) / subdiv;
        float angle = rate * pi * 2.0f;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        // 内周頂点
        float inX = offsetX + cosA * innerRadiusX;
        float inY = offsetY + sinA * innerRadiusY;
        float uIn = cosA * 0.5f * (innerRadiusX / outerRadiusX) + 0.5f;
        float vIn = sinA * 0.5f * (innerRadiusY / outerRadiusY) + 0.5f;
        resource_->vertexDataList_.push_back({{inX, inY, 0.0f, 1.0f}, {uIn, vIn}, {0.0f, 0.0f, -1.0f}});

        // 外周頂点
        float outX = offsetX + cosA * outerRadiusX;
        float outY = offsetY + sinA * outerRadiusY;
        float uOut = cosA * 0.5f + 0.5f;
        float vOut = sinA * 0.5f + 0.5f;
        resource_->vertexDataList_.push_back({{outX, outY, 0.0f, 1.0f}, {uOut, vOut}, {0.0f, 0.0f, -1.0f}});
    }

    for (uint32_t i = 0; i < subdiv; ++i) {
        uint32_t p0 = i * 2;     // 内周 i
        uint32_t p1 = i * 2 + 1; // 外周 i
        uint32_t p2 = i * 2 + 2; // 内周 i+1
        uint32_t p3 = i * 2 + 3; // 外周 i+1

        // トライアングル1
        resource_->indexDataList_.push_back(p0);
        resource_->indexDataList_.push_back(p1);
        resource_->indexDataList_.push_back(p2);
        // トライアングル2
        resource_->indexDataList_.push_back(p2);
        resource_->indexDataList_.push_back(p1);
        resource_->indexDataList_.push_back(p3);
    }
}

void Primitive2DObject::BuildLine() {
    // Lineの仕様: size_.x を長さ、thickness_ を太さとする
    // pivotは長さに対する基準（例：pivot.x=0なら始点基準、0.5なら中心基準）
    // pivot.y は太さに対する基準

    float length = size_.x;
    float thick = thickness_;

    float left = -length * pivot_.x;
    float right = length * (1.0f - pivot_.x);
    float top = -thick * pivot_.y;
    float bottom = thick * (1.0f - pivot_.y);

    resource_->vertexDataList_ = {
        {{left, top, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},    // 左上
        {{right, top, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},   // 右上
        {{left, bottom, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}, // 左下
        {{right, bottom, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}} // 右下
    };

    resource_->indexDataList_ = {0, 1, 2, 2, 1, 3};
}
