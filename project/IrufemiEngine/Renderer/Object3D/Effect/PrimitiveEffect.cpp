#include "PrimitiveEffect.h"
#include "../../../Engine/Manager/PrimitiveManager.h"
#include "../../../Engine/Manager/DrawManager.h"
#ifdef USE_IMGUI
#include "../../../../externals/imgui/imgui.h"
#endif
#include <vector>

TextureManager* PrimitiveEffect::textureManager_ = nullptr;

void PrimitiveEffect::Initialize(Camera* camera, PrimitiveType type, const std::string& texturePath) {
    primitive_.Initialize(camera, type, texturePath);

    // デフォルトでライティングをOFFに（エフェクトのため）
    primitive_.GetMaterial().enableLighting = false;

    // 半透明対応のためアルファ1にしておく
    primitive_.GetMaterial().color.w = 1.0f;

    // エフェクトなので影はデフォルトでオフ
    primitive_.SetCastShadows(false);

    isCylinderMode_ = (type == PrimitiveType::Cylinder);
}

void PrimitiveEffect::Update(float deltaTime) {
    // UVスクロール更新
    uvScrollOffset_.x += uvScrollSpeed_.x * deltaTime;
    uvScrollOffset_.y += uvScrollSpeed_.y * deltaTime;

    // 行列の構築 (Scale -> Rotate(None) -> Translate)
    if (primitive_.GetMesh().resource) {
        primitive_.GetMesh().resource->uvTransform_.translate = { uvScrollOffset_.x, uvScrollOffset_.y, 0.0f };
        primitive_.GetMesh().resource->uvTransform_.scale = { uvScale_.x, uvScale_.y, 1.0f };
    }

    // 基本コンポーネントの更新
    primitive_.Update();
}

void PrimitiveEffect::Draw() {
    // CullModeの切り替えについては、DrawManager側に投げるか、
    // 現在は PrimitiveObjects3DClass 内の描画関数で固定Pipelineが使われる場合は
    // PipelineManager に投げる必要があります。
    // （ここではDrawManagerに何らかのカスタムCullを設定する、もしくは直接PSOを分ける想定）
    // 現状は Effect 専用Pipelineとして DrawManager 側の API (無ければ通常描画) を用います。

    // primitive_.Draw(); にCullModeを反映させる仕組みが BaseResource などにあれば適用。
    // 今回はDrawManagerの仕様に応じて、標準Object用と解釈しそのままDrawします。
    // TODO: DrawManager に合わせたパイプラインステート切り替えのフック
    
    // 一時的にカリング設定を保持・適用できればベター
    // bool tmpCull = primitive_.IsCullingEnabled();
    // primitive_.SetCullingEnabled(cullMode_ != CullMode::None);

    primitive_.Draw();
}

void PrimitiveEffect::SetCylinderParams(float bottomRadius, float topRadius, float height, uint32_t segments, bool hasTop, bool hasBottom, bool centered) {
    cylBottomRadius_ = bottomRadius;
    cylTopRadius_    = topRadius;
    cylHeight_       = height;
    cylSegments_     = segments;
    cylHasTop_       = hasTop;
    cylHasBottom_    = hasBottom;
    cylCentered_     = centered;
    isCylinderMode_  = true;

    // カスタムで生成したデータを MeshComponent の resource に流し直す
    PrimitiveData customData = PrimitiveManager::CreateCylinder(bottomRadius, topRadius, height, segments, hasTop, hasBottom, centered);
    
    // 生成したカスタムデータをObject3DResourceへ転送する処理
    // 新しく追加された ReinitializeMesh メソッドを使用し、動的にリソースを再生成して置き換えます。
    primitive_.ReinitializeMesh(customData);
}

void PrimitiveEffect::Debug(const char* label) {
#ifdef USE_IMGUI
    ImGui::Begin(label);
    
    ImGui::Text("--- Transform ---");
    Vector3 pos = primitive_.GetTransform().transform.translate;
    if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) { primitive_.SetPosition(pos); }
    
    Vector3 rot = primitive_.GetTransform().transform.rotate;
    if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) { primitive_.SetRotate(rot); }
    
    Vector3 scl = primitive_.GetTransform().transform.scale;
    if (ImGui::DragFloat3("Scale", &scl.x, 0.1f)) { primitive_.SetScale(scl); }

    ImGui::Separator();
    ImGui::Text("--- Effect Parameters ---");
    ImGui::DragFloat2("UV Scroll Speed", &uvScrollSpeed_.x, 0.1f);
    ImGui::DragFloat("Alpha Ref (discard)", &primitive_.GetMaterial().alphaReference, 0.05f, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("--- UV Flip ---");
    bool flipX = (uvScale_.x < 0.0f);
    if (ImGui::Checkbox("Flip U", &flipX)) { uvScale_.x = flipX ? -1.0f : 1.0f; }
    ImGui::SameLine();
    bool flipY = (uvScale_.y < 0.0f);
    if (ImGui::Checkbox("Flip V", &flipY)) { uvScale_.y = flipY ? -1.0f : 1.0f; }

    ImGui::Separator();
    ImGui::Text("--- Texture Material ---");
    
    if (textureManager_ && !textureManager_->GetTextureNamesForDebug().empty()) {
        auto textureNames = textureManager_->GetTextureNamesForDebug();
        std::vector<const char*> namesCStr;
        for (const auto& name : textureNames) {
            namesCStr.push_back(name.c_str());
        }
        
        // 現在のテクスチャ名からインデックスを逆引き
        if (selectedTextureIndex_ == -1) {
            std::string currentTexture = primitive_.GetMaterial().texturePath;
            for (int i = 0; i < textureNames.size(); ++i) {
                if (textureNames[i] == currentTexture) {
                    selectedTextureIndex_ = i;
                    break;
                }
            }
            if (selectedTextureIndex_ == -1) selectedTextureIndex_ = 0;
        }

        if (ImGui::Combo("Texture", &selectedTextureIndex_, namesCStr.data(), (int)namesCStr.size())) {
            primitive_.SetTexture(textureNames[selectedTextureIndex_]);
        }
    } else {
        // Fallback for string input
        char texBuffer[256];
        std::string currentTexture = primitive_.GetMaterial().texturePath;
        strncpy_s(texBuffer, currentTexture.c_str(), sizeof(texBuffer));
        if (ImGui::InputText("Texture Path", texBuffer, sizeof(texBuffer))) {
            primitive_.SetTexture(texBuffer);
        }
    }

        ImGui::Separator();
        ImGui::Text("--- Culling ---");
        int cull = static_cast<int>(cullMode_);
        if (ImGui::Combo("Cull Mode", &cull, "None\0Front\0Back\0")) {
            cullMode_ = static_cast<PSOManager::CullMode>(cull);
        }

    if (isCylinderMode_) {
        ImGui::Separator();
        ImGui::Text("--- Cylinder Shape ---");
        bool changed = false;
        changed |= ImGui::DragFloat("Bottom Radius", &cylBottomRadius_, 0.05f, 0.01f, 10.0f);
        changed |= ImGui::DragFloat("Top Radius", &cylTopRadius_, 0.05f, 0.0f, 10.0f);
        changed |= ImGui::DragFloat("Height", &cylHeight_, 0.1f, 0.1f, 20.0f);
        changed |= ImGui::Checkbox("Has Top", &cylHasTop_);
        changed |= ImGui::Checkbox("Has Bottom", &cylHasBottom_);
        changed |= ImGui::Checkbox("Centered", &cylCentered_);

        if (changed) {
            SetCylinderParams(cylBottomRadius_, cylTopRadius_, cylHeight_, cylSegments_, cylHasTop_, cylHasBottom_, cylCentered_);
        }
    }

    ImGui::End();
#endif
}


void PrimitiveEffect::SyncBeforeDraw() {
    primitive_.SyncBeforeDraw();
}

