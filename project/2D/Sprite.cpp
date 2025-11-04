#define NOMINMAX
#include "Sprite.h"

#include "manager/DebugUI.h"
#include "manager/TextureManager.h"
#include "manager/DrawManager.h"
#include "function/Math.h"
#include "function/Function.h"
#include "externals/imgui/imgui.h"

#include <algorithm>

TextureManager* Sprite::textureManager_ = nullptr;
DrawManager* Sprite::drawManager_ = nullptr;
DebugUI* Sprite::ui_ = nullptr;

void Sprite::Initialize(Camera* camera, const std::string& textureName) {
    this->camera_ = camera;
    resource_ = std::make_unique<D3D12ResourceUtil>();

    // 頂点はユニットクワッド(0..1)に統一（サイズはscaleで与える）
    // 左下
    resource_->vertexDataList_.push_back({ { 0.0f,1.0f,0.0f,1.0f }, { 0.0f,1.0f } ,{0.0f,0.0f,-1.0f} });
    // 左上
    resource_->vertexDataList_.push_back({ { 0.0f,0.0f,0.0f,1.0f }, { 0.0f,0.0f},{0.0f,0.0f,-1.0f} });
    // 右下
    resource_->vertexDataList_.push_back({ { 1.0f,1.0f,0.0f,1.0f }, { 1.0f,1.0f } ,{0.0f,0.0f,-1.0f} });
    // 右上
    resource_->vertexDataList_.push_back({ { 1.0f,0.0f,0.0f,1.0f }, { 1.0f,0.0f } ,{0.0f,0.0f,-1.0f} });

    resource_->indexDataList_.push_back(0);
    resource_->indexDataList_.push_back(1);
    resource_->indexDataList_.push_back(2);
    resource_->indexDataList_.push_back(1);
    resource_->indexDataList_.push_back(3);
    resource_->indexDataList_.push_back(2);

    // リソースのメモリを確保
    resource_->CreateResource();

    // 書き込めるようにする
    resource_->Map();

    //頂点バッファ

    resource_->vertexBufferView_ = D3D12_VERTEX_BUFFER_VIEW{};

    resource_->vertexBufferView_.BufferLocation = resource_->vertexResource_->GetGPUVirtualAddress();
    resource_->vertexBufferView_.StrideInBytes = sizeof(VertexData);
    resource_->vertexBufferView_.SizeInBytes = sizeof(VertexData) * static_cast<UINT>(resource_->vertexDataList_.size());

    std::copy(resource_->vertexDataList_.begin(), resource_->vertexDataList_.end(), resource_->vertexData_);

    /*頂点インデックス*/

    ///Index用のあれやこれやを作る

    resource_->indexBufferView_ = D3D12_INDEX_BUFFER_VIEW{};
    //リソースの先頭のアドレスから使う
    resource_->indexBufferView_.BufferLocation = resource_->indexResource_->GetGPUVirtualAddress();
    //使用するリソースのサイズはインデックス6つ分のサイズ
    resource_->indexBufferView_.SizeInBytes = sizeof(uint32_t) * 6;
    //インデックスはint32_tとする
    resource_->indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    ///IndexResourceにデータを書き込む
    //インデックスリソースにデータを書き込む

    std::copy(resource_->indexDataList_.begin(), resource_->indexDataList_.end(), resource_->indexData_);

    // VB/IB作成とMapが済んだ後に一度アンカーを頂点に反映しておく
    ApplyAnchorToVertices();

    // --- 初回の行列計算（アンカー分は頂点側で処理するので引かない） ---
    Vector3 pos = resource_->transform_.translate;
    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, pos);
    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };

    //マテリアル

    resource_->materialData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->materialData_->enableLighting = false;
    resource_->materialData_->hasTexture = true;
    resource_->materialData_->lightingMode = 2;
    resource_->materialData_->uvTransform = Math::MakeIdentity4x4();


    resource_->directionalLightData_->color = { 1.0f,1.0f,1.0f,1.0f };
    resource_->directionalLightData_->direction = { 0.0f,-1.0f,0.0f, };
    resource_->directionalLightData_->intensity = 1.0f;

    // --- 必ず指定テクスチャをロード＆ハンドル取得 ---
    if (textureManager_) {
        // 指定名で必ずロード/取得（未ロードならロード）
        resource_->textureHandle_ = textureManager_->GetTextureHandle(textureName);

        // テクスチャサイズを直接取得して反映
        uint32_t tw = 0, th = 0;
        if (textureManager_->GetTextureSize(textureName, tw, th) && tw > 0 && th > 0) {
            textureSize_ = { static_cast<float>(tw), static_cast<float>(th) };
            SetSize(textureSize_.x, textureSize_.y);

            // 行列を更新（サイズ反映後）
            Vector3 pos3 = resource_->transform_.translate;
            resource_->transformationMatrix_.world = Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, pos3);
            resource_->transformationMatrix_.WVP   = Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
            *resource_->transformationData_        = { resource_->transformationMatrix_.WVP, resource_->transformationMatrix_.world };
        }

        // デバッグUI（コンボ）用に selectedTextureIndex_ を既存ロジックで決める
        auto textureNames = textureManager_->GetTextureNames();
        std::sort(textureNames.begin(), textureNames.end());
        if (!textureNames.empty()) {
            auto it = std::find(textureNames.begin(), textureNames.end(), textureName);
            selectedTextureIndex_ = (it != textureNames.end())
                ? static_cast<int>(std::distance(textureNames.begin(), it))
                : 0;
        }
    }
}

void Sprite::Update(const bool& debug, const char * spriteName) {

    if (debug) {
#if defined(_DEBUG) || defined(DEVELOPMENT)
        std::string name = std::string("Sprite: ") + spriteName;

        //ImGui

        //カメラウィンドウを作り出す
        ImGui::Begin(name.c_str());

        ui_->DebugTransform2D(resource_->transform_);

        ui_->DebugMaterialBy2D(resource_->materialData_);

        ui_->DebugTexture(resource_.get(), selectedTextureIndex_);

        ui_->DebugUvTransform(resource_->uvTransform_);

        ImGui::Checkbox("Flip X", &isFlipX_);

        ImGui::Checkbox("Flip Y", &isFlipY_);

        ImGui::Separator();
        // Anchor/Size 操作
        float a[2] = { anchor_.x, anchor_.y };
        if (ImGui::SliderFloat2("Anchor (0..1)", a, 0.0f, 1.0f)) {
            SetAnchor(a[0], a[1]);
        }
        if (ImGui::SmallButton("TopLeft (0,0)")) { SetAnchor(0.0f, 0.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Center (0.5,0.5)")) { SetAnchor(0.5f, 0.5f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("BottomRight (1,1)")) { SetAnchor(1.0f, 1.0f); }

        float sz[2] = { size_.x, size_.y };
        if (ImGui::DragFloat2("Size (px)", sz, 1.0f, 1.0f, 8192.0f)) {
            SetSize(sz[0], sz[1]);
        }

        // 実効矩形の確認表示（アンカー適用後のTopLeft/BottomRight）
        const float left = resource_->transform_.translate.x - anchor_.x * size_.x;
        const float top = resource_->transform_.translate.y - anchor_.y * size_.y;
        const float right = left + size_.x;
        const float bottom = top + size_.y;
        ImGui::Text("Rect L=%.1f T=%.1f R=%.1f B=%.1f", left, top, right, bottom);

        // 切り出しUI
        if (ImGui::CollapsingHeader("Texture Rect (px)")) {
            bool enabled = useTexRect_;
            if (ImGui::Checkbox("Enable", &enabled)) {
                useTexRect_ = enabled;
                if (!useTexRect_) ClearTextureRect();
            }
            int lt[2] = { static_cast<int>(texRectLeftTop_.x), static_cast<int>(texRectLeftTop_.y) };
            int szpx[2] = { static_cast<int>(texRectSize_.x), static_cast<int>(texRectSize_.y) };
            bool changed = false;
            changed |= ImGui::DragInt2("LeftTop", lt, 1);
            changed |= ImGui::DragInt2("Size", szpx, 1);
            if (changed && enabled) {
                SetTextureRectPixels(lt[0], lt[1], std::max(1, szpx[0]), std::max(1, szpx[1]), false);
            }
            if (ImGui::SmallButton("Reset Full")) {
                ClearTextureRect();
            }
            ImGui::Text("TexSize: (%.0f, %.0f)", textureSize_.x, textureSize_.y);
        }

        //入力終了
        ImGui::End();

#endif // _DEBUG

    }

    // DebugUI でのテクスチャ選択変更に追随してサイズを更新
    if (textureManager_) {
        auto names = textureManager_->GetTextureNames();
        std::sort(names.begin(), names.end());
        if (!names.empty()) {
            selectedTextureIndex_ = std::clamp(selectedTextureIndex_, 0, static_cast<int>(names.size()) - 1);
            uint32_t tw = 0, th = 0;
            if (textureManager_->GetTextureSize(names[selectedTextureIndex_], tw, th) && tw > 0 && th > 0) {
                textureSize_ = { static_cast<float>(tw), static_cast<float>(th) };

                // 追加: 範囲指定中なら新サイズに合わせて再クランプ
                if (useTexRect_) {
                    SetTextureRectPixels(
                        static_cast<int>(texRectLeftTop_.x),
                        static_cast<int>(texRectLeftTop_.y),
                        static_cast<int>(texRectSize_.x),
                        static_cast<int>(texRectSize_.y),
                        /*autoResize=*/false);
                }
            }
        }
    }

    // アンカーの変更を頂点へ反映
    ApplyAnchorToVertices();

    // 行列更新
    Vector3 pos = resource_->transform_.translate;
    resource_->transformationMatrix_.world =
        Math::MakeAffineMatrix(resource_->transform_.scale, resource_->transform_.rotate, pos);
    resource_->transformationMatrix_.WVP =
        Math::Multiply(resource_->transformationMatrix_.world, camera_->GetOrthographicMatrix());
    *resource_->transformationData_ = { resource_->transformationMatrix_.WVP,resource_->transformationMatrix_.world };

    // ---- UV 変換（flip → crop → userUV）----
     // userUV: 既存の uvTransform（回転/スクロール）
    Matrix4x4 userUV =
        Math::MakeAffineMatrix(resource_->uvTransform_.scale, resource_->uvTransform_.rotate, resource_->uvTransform_.translate);

    // crop: px指定 → 正規化UVに変換
    Matrix4x4 cropUV = Math::MakeIdentity4x4();
    if (useTexRect_ && textureSize_.x > 0.0f && textureSize_.y > 0.0f) {
        float u0 = texRectLeftTop_.x / textureSize_.x;
        float v0 = texRectLeftTop_.y / textureSize_.y;
        float du = texRectSize_.x / textureSize_.x;
        float dv = texRectSize_.y / textureSize_.y;
        cropUV = Math::MakeAffineMatrix(Vector3{ du, dv, 1.0f }, Vector3{ 0.0f,0.0f,0.0f }, Vector3{ u0, v0, 0.0f });
    }

    // flip を最初に、次に crop、最後に userUV を適用
    Vector3 flipScale{ isFlipX_ ? -1.0f : 1.0f, isFlipY_ ? -1.0f : 1.0f, 1.0f };
    Vector3 flipTrans{ isFlipX_ ? 1.0f : 0.0f, isFlipY_ ? 1.0f : 0.0f, 0.0f };
    Matrix4x4 flipUV = Math::MakeAffineMatrix(flipScale, Vector3{ 0.0f,0.0f,0.0f }, flipTrans);

    // mul(row, M) を想定: input * (flip * crop * userUV)
    Matrix4x4 base = Math::Multiply(cropUV, userUV);
    resource_->materialData_->uvTransform = Math::Multiply(flipUV, base);

    resource_->directionalLightData_->direction = Math::Normalize(resource_->directionalLightData_->direction);

}

void Sprite::Draw() {
    drawManager_->DrawSprite(this);
}

void Sprite::SetSize(const float& width, const float& height) {
    size_.x = width;
    size_.y = height;
    // 実サイズはscaleで表現
    resource_->transform_.scale = { size_.x, size_.y, 1.0f };
}

void Sprite::SetAnchor(const float& ax, const float& ay) {
    anchor_.x = ax;
    anchor_.y = ay;
}

void Sprite::SetPosition(const float& x, const float& y, const float& z) {
    resource_->transform_.translate.x = x;
    resource_->transform_.translate.y = y;
    resource_->transform_.translate.z = z;
    // 毎フレーム Update() で WVP を再計算しているため、ここでの再計算は不要
}

const Vector2 Sprite::GetPosition2D() const {
    return { resource_->transform_.translate.x, resource_->transform_.translate.y };
}

void Sprite::ApplyAnchorToVertices() {
    // アンカーによるローカル頂点のずらし（資料通り）
    const float left = 0.0f - anchor_.x;
    const float right = 1.0f - anchor_.x;
    const float top = 0.0f - anchor_.y;
    const float bottom = 1.0f - anchor_.y;

    // 頂点の並びは初期生成と同じインデックスに合わせる
    // 0: 左下, 1: 左上, 2: 右下, 3: 右上
    resource_->vertexData_[0].position = { left,  bottom, 0.0f, 1.0f };
    resource_->vertexData_[1].position = { left,  top,    0.0f, 1.0f };
    resource_->vertexData_[2].position = { right, bottom, 0.0f, 1.0f };
    resource_->vertexData_[3].position = { right, top,    0.0f, 1.0f };
}

// テクスチャ範囲指定API 
bool Sprite::SetTextureRectPixels(int x, int y, int w, int h, bool autoResize) {
    if (textureSize_.x <= 0.0f || textureSize_.y <= 0.0f) return false;

    const int texW = static_cast<int>(textureSize_.x);
    const int texH = static_cast<int>(textureSize_.y);

    int sx = std::clamp(x, 0, texW);
    int sy = std::clamp(y, 0, texH);
    int ex = std::clamp(sx + std::max(w, 0), 0, texW);
    int ey = std::clamp(sy + std::max(h, 0), 0, texH);
    if (ex <= sx || ey <= sy) return false;

    texRectLeftTop_ = { static_cast<float>(sx), static_cast<float>(sy) };
    texRectSize_ = { static_cast<float>(ex - sx), static_cast<float>(ey - sy) };
    useTexRect_ = true;

    if (autoResize) {
        SetSize(texRectSize_.x, texRectSize_.y);
    }
    return true;
}

void Sprite::ClearTextureRect() {
    useTexRect_ = false;
    texRectLeftTop_ = { 0.0f, 0.0f };
    texRectSize_ = { 0.0f, 0.0f };
}

// 追加: 現在選択されているテクスチャの解像度でスプライトサイズを合わせる
void Sprite::AdjustTextureSize() {
    if (!textureManager_) return;

    auto names = textureManager_->GetTextureNames();
    std::sort(names.begin(), names.end());
    if (names.empty()) return;

    selectedTextureIndex_ = std::clamp(selectedTextureIndex_, 0, static_cast<int>(names.size()) - 1);

    uint32_t tw = 0, th = 0;
    if (textureManager_->GetTextureSize(names[selectedTextureIndex_], tw, th) && tw > 0 && th > 0) {
        textureSize_ = { static_cast<float>(tw), static_cast<float>(th) };
        SetSize(textureSize_.x, textureSize_.y);
    }
}