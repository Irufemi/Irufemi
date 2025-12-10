// Field.cpp
#include "Field.h"

// ★ ここはあなたの環境に合わせて変更してね
// 例: "Math/Vector3.h" とか "engine/math/Vector3.h" とか
#include "math/Vector3.h"

#include <algorithm> // std::min, std::max
#include <cmath>
#include <cstdlib> // rand()
#include "engine/irufemiEngine.h"

// -------------------------
// 便利関数（このファイル内だけで使用）
// -------------------------

namespace {
// 0.0〜1.0 にクランプ
float Clamp01(float v) {
  if (v < 0.0f) {
    return 0.0f;
  }
  if (v > 1.0f) {
    return 1.0f;
  }
  return v;
}

// 0.0〜1.0 の乱数（雑でOKな版）
float Random01() {
  return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

constexpr float kPi = 3.1415926535f;
} // namespace

void Field::StartFadeToBlack(float durationSec)
{
    // durationSec 秒かけて 0→1 にする
    if (durationSec <= 0.0f) {
        fade_ = 1.0f;
        fading_ = false;
        fadeSpeed_ = 0.0f;
        return;
    }

    fading_ = true;
    fadeSpeed_ = 1.0f / durationSec;
}

//void Field::Update(float deltaTime)
//{
//    if (!fading_) { return; }
//
//    fade_ += fadeSpeed_ * deltaTime;
//    if (fade_ >= 1.0f) {
//        fade_ = 1.0f;
//        fading_ = false; // 完了
//    }
//}



// -------------------------
// Field 本体
// -------------------------

void Field::Initialize(IrufemiEngine *engine, Camera *camera) {
  engine_ = engine;
  camera_ = camera;

  // ★ 今はロジックだけ。描画用メッシュや Region などは
  //    後で「見た目を作るフェーズ」で追加する。
  //
  // 例:
  // fieldRegion_ = new Region();
  // fieldRegion_->Initialize(...);
  // ★ フィールド円柱の論理情報を設定
  fieldInfo_.radius = radius_; // そのままフィールド半径に
  fieldInfo_.height = 0.5f;    // 厚み（お好み：0.3〜1.0くらい）
  // 上面が y = 0 に来るように、中心を -height/2 に置く
  fieldInfo_.center = {0.0f, -fieldInfo_.height * 0.5f - 0.5f, 0.0f};

  // CylinderClass に渡す
  fieldCylinder_.SetInfo(fieldInfo_);

  // ★ カメラ＆テクスチャ指定で初期化
  // 砂用テクスチャがまだなければ、とりあえず uvChecker でもOK
  fieldCylinder_.Initialize(camera, "resources/uvChecker.png");

  assert(engine);

  // --- フィールド用定数バッファ初期値 ---
  fieldCB_.timeSec = 0.0f;
  fieldCB_.blackFade = 0.0f;
  fieldCB_.pad[0] = 0.0f;
  fieldCB_.pad[1] = 0.0f;

  // ★ IrufemiEngine 経由で定数バッファを作成
  fieldCBResource_ = engine_->CreateBufferResource(sizeof(FieldCBData));
  assert(fieldCBResource_);

  // 一度初期値を書き込んでおく
  void* mapped = nullptr;
  fieldCBResource_->Map(0, nullptr, &mapped);
  memcpy(mapped, &fieldCB_, sizeof(FieldCBData));
  fieldCBResource_->Unmap(0, nullptr);
}

void Field::Update(float deltaTime) {

    // -------------------------
    // ① 黒フェードアニメーション
    // -------------------------
    if (fading_) {
        fade_ += fadeSpeed_ * deltaTime;
        if (fade_ >= 1.0f) {
            fade_ = 1.0f;
            fading_ = false; // 完了
        }
    }

    // -------------------------
    // ② 定数バッファに反映
    // -------------------------
     // --- 定数バッファへ書き込み ---
    if (fieldCBResource_) {
        fieldCB_.blackFade = fade_;              // 0.0〜1.0
        fieldCB_.timeSec += 1.0f / 60.0f;       // 必要なら適当に

        void* mapped = nullptr;
        fieldCBResource_->Map(0, nullptr, &mapped);
        memcpy(mapped, &fieldCB_, sizeof(FieldCBData));
        fieldCBResource_->Unmap(0, nullptr);
    }

  // 今のところフィールド自身は時間で変化しないので何もしない。
  // 砂嵐を動かしたり、ステージギミックを追加したくなったらここに処理を書く。
  fieldCylinder_.SetInfo(fieldInfo_);
  fieldCylinder_.Update();
}

void Field::Draw() {
  // ★ 今は空でOK。
  //   円形メッシュ＋夜砂マテリアルが用意できたら、ここで描画を呼ぶ。
  //
  // if (fieldRegion_) {
  //     fieldRegion_->Draw();
  // }

   // いつものフィールド用 PSO を適用
    engine_->ApplyFieldCylinderPSO();

    // --- フィールド用定数バッファ (b5) を root パラメータ 9 にバインド ---
    if (fieldCBResource_) {
        engine_->GetCommandList()->SetGraphicsRootConstantBufferView(
            9, // ← RootSignature / PSO で b5 を束ねたスロットに合わせる
            fieldCBResource_->GetGPUVirtualAddress());
    }

    // 円柱描画
    fieldCylinder_.Draw();
  fieldCylinder_.Draw();
}

// -------------------------
// ステージパラメータ
// -------------------------

void Field::SetRadius(float r) {
  // 半径が 0 以下にならないように最低値を入れておく
  radius_ = (r > 0.01f) ? r : 0.01f;
  fieldInfo_.radius = radius_;
  fieldCylinder_.SetRadius(radius_);
}

float Field::GetRadius() const { return radius_; }

void Field::SetFadeRates(float startRate, float endRate) {
  // 0〜1 にクランプしておく
  fadeStartRate_ = Clamp01(startRate);
  fadeEndRate_ = Clamp01(endRate);

  // start > end になっていたら、入れ替えておく
  if (fadeStartRate_ > fadeEndRate_) {
    std::swap(fadeStartRate_, fadeEndRate_);
  }
}

void Field::ResetFade()
{
    fade_ = 0.0f;      // or blackFade_ = 0.0f;
    fadeSpeed_ = 0.0f;
    fading_ = false;

    fieldCB_.blackFade = 0.0f;
    if (fieldCBResource_) {
        void* mapped = nullptr;
        fieldCBResource_->Map(0, nullptr, &mapped);
        std::memcpy(mapped, &fieldCB_, sizeof(FieldCBData));
        fieldCBResource_->Unmap(0, nullptr);
    }
}

//void Field::SetHeightScale(float scale) { heightScale_ = scale; }

// -------------------------
// 地形・境界関連
// -------------------------

// (x, z) から地面の高さ y を求める
//float Field::GetHeight(float x, float z) const {
//  // 中心からの距離 r
//  float r = std::sqrt(x * x + z * z);
//
//  // 半径 0.8 * radius_ で最大高さになるようなパラメータ
//  float maxR = radius_ * 0.8f;
//  if (maxR <= 0.0f) {
//    return 0.0f;
//  }
//
//  // 0〜1 の割合に正規化
//  float t = Clamp01(r / maxR);
//
//  // t^2 で、中心が低くて外側が少し高い「ゆるい砂丘」カーブにする
//  float h = t * t;
//
//  return h ;
//}
//
//float Field::GetHeight(const Vector3 &pos) const {
//  return GetHeight(pos.x, pos.z);
//}

// 円ステージの内側に位置を収める（はみ出したら円周上にクランプ）
Vector3 Field::ClampInside(const Vector3 &pos) const {
  Vector3 result = pos;

  float r = std::sqrt(result.x * result.x + result.z * result.z);

  // 半径を超えていたら円周上に押し戻す
  if (r > radius_ && r > 0.0f) {
    float k = radius_ / r;
    result.x *= k;
    result.z *= k;
  }

  return result;
}

// -------------------------
// スポーン用ヘルパ
// -------------------------

// 円の中のランダム位置（均等分布）
Vector3 Field::GetRandomPointInField() const {
  // 半径 r の円内一様分布にするために、sqrt を使う
  float u = Random01();
  float r = radius_ * std::sqrt(u);
  float theta = Random01() * 2.0f * kPi;

  float x = r * std::cos(theta);
  float z = r * std::sin(theta);
  float y = 0.0f;

  return Vector3{x, y, z};
}

// 内側/外側を指定したリング領域内ランダム
Vector3 Field::GetRandomPointInRing(float innerRadius,
                                    float outerRadius) const {
  // 半径をクランプしておく
  innerRadius = (std::max)(0.0f, innerRadius);
  outerRadius = (std::min)(radius_, outerRadius);

  if (outerRadius < innerRadius) {
    std::swap(innerRadius, outerRadius);
  }

  // inner==outer の場合は、その円周上に一様分布
  if (outerRadius <= 0.0f) {
    // どうしようもないので原点返し
    return Vector3{0.0f, 0.0f, 0.0f};
  }

  float inner2 = innerRadius * innerRadius;
  float outer2 = outerRadius * outerRadius;

  float u = Random01();
  // 半径^2 を線形補間してから sqrt するとリング内一様分布になる
  float r2 = inner2 + (outer2 - inner2) * u;
  float r = std::sqrt(r2);

  float theta = Random01() * 2.0f * kPi;

  float x = r * std::cos(theta);
  float z = r * std::sin(theta);
  float y = 0.0f;
  return Vector3{x, y, z};
}

// -------------------------
// 外周フェード用（描画シェーダに渡す係数）
// -------------------------

// 戻り値：0.0 = 中央, 1.0 = 外周
float Field::CalcFade(float x, float z) const {
  float r = std::sqrt(x * x + z * z);

  float start = radius_ * fadeStartRate_;
  float end = radius_ * fadeEndRate_;

  if (end <= start) {
    // おかしな設定になっていたらフェードなし扱い
    return 0.0f;
  }

  float t = (r - start) / (end - start);
  return Clamp01(t);
}
