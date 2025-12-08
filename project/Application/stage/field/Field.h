// Field.h
#pragma once
#include"3D/CylinderClass.h"
// 必要に応じて自分のエンジンのヘッダに合わせて書き換えてね
// 例: #include "Vector3.h" とか "Math.h" とか
struct Vector3;
class Camera;
class IrufemiEngine;
class CylinderClass;
// class Region;   // 描画に Region 使うなら前方宣言してOK

class Field
{
public:
    Field() = default;
    ~Field() = default;

    // --------------------
    // 基本ライフサイクル
    // --------------------
    void Initialize(IrufemiEngine* engine, Camera* camera);
    void Update(float deltaTime);
    void Draw();

    // --------------------
    // ステージパラメータ
    // --------------------
    void  SetRadius(float r);
    float GetRadius() const;

    // 必要なら外からフェード設定をいじれるように
    void  SetFadeRates(float startRate, float endRate);
    //void  SetHeightScale(float scale);

    // --------------------
    // 地形・境界関連
    // --------------------

    // 地面の高さ取得（x,z から y を返す）
    //float  GetHeight(float x, float z) const;
    //float  GetHeight(const Vector3& pos) const;

    // ステージ円の内側に位置を収める（はみ出したら円周上にクランプ）
    Vector3 ClampInside(const Vector3& pos) const;

    // --------------------
    // スポーン用ヘルパ
    // --------------------

    // 円の中のランダム位置（プレイヤー・岩・敵のスポーンに使える）
    Vector3 GetRandomPointInField() const;

    // 内側/外側を指定したリング領域内ランダム
    Vector3 GetRandomPointInRing(float innerRadius, float outerRadius) const;

    // --------------------
    // 外周フェード用（シェーダ or 定数バッファ用）
    // --------------------
    // 0.0 = 中央, 1.0 = 外周付近
    float CalcFade(float x, float z) const;

private:
    // ステージ形状パラメータ
    float radius_ = 8.0f;   // 円ステージ半径
   // float heightScale_ = 0.15f;   // 砂丘の盛り上がり量
    float fadeStartRate_ = 0.75f;   // フェード開始(半径に対する割合)
    float fadeEndRate_ = 1.0f;    // フェード終了(半径に対する割合)

    // エンジン・カメラ参照（必要なら）
    IrufemiEngine* engine_ = nullptr;
    Camera* camera_ = nullptr;

    // 描画用の何か（Region や Model 等）
    // ここはあなたのエンジンに合わせて後で決めよう
    // Region* fieldRegion_ = nullptr;
    // ------------------------
    // ★ 描画用メンバを追加
    // ------------------------
    // ★ フィールド用円柱
    Cylinder       fieldInfo_;      // 論理情報（中心・半径・高さ）
    CylinderClass  fieldCylinder_;  // 描画＆変換
};
