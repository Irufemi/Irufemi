#pragma once

#include "../math/Vector2.h"
#include "../math/Vector3.h"
#include "../math/Matrix4x4.h"
#include <numbers>

class Camera {
private: // メンバ変数

    //カメラの位置。ワールド座標。
    Vector3 translate_ = { 0.0f,0.0f,-50.0f };

    //カメラの回転角度
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };

    //カメラの拡縮(ここはいじらない。)
    const Vector3 scale_ = { 1.0f,1.0f,1.0f };

#pragma region 正射影行列を構成する変数(カメラで映す空間の範囲)

    // カメラで映す空間の左端のX座標
    float left_ = 0.0f;

    //カメラで映す空間の上端のY座標
    float top_ = 0.0f;

    //カメラで映す空間の右端のX座標
    float right_ = 1280.0f;

    //カメラで映す空間の下端のY座標
    float bottom_ = 720.0f;

    //近平面。ここではz軸が奥行きになるため一番手前(面なので0だと面が点になって映らない。できるだけ全部が映る遠いところがいい。大体目安は0.1程度)。
    float nearClip_ = 0.0f;

    //遠平面。ここではz軸が奥行きになるため遠さを表す。
    float farClip_ = 100.0f;

#pragma endregion

#pragma region 透視投影行列を構成する変数(カメラで映す空間の範囲)

    //垂直方向視野角
    float fovAngleY_ = 45.0f * 3.141592654f / 180.0f;

    //ビューポートのアスペクト比
    float aspectRatio_ = 16.0f / 9.0f;

    //深度限界(手前側)
    float nearZ_ = 0.1f;

    //深度限界(奥側)
    float farZ_ = 1000.0f;

#pragma endregion

#pragma region ビューポート行列を構成する変数(ウィンドウ上で映す範囲)

    //画面上で映す横幅
    float width_ = 1280.0f;

    //画面上で映す高さ
    float height_ = 720.0f;

    //ウィンドウに映す範囲の左上の座標
    Vector2 leftTop_ = { 0.0f,0.0f };

    //mindepth(最小深度値)
    float minDepth_ = 0.0f;

    //maxDepth(最大深度値)
    float maxDepth_ = 1.0f;

#pragma endregion

    //ワールド行列
    Matrix4x4 worldMatrix_{};

    //ビュー行列
    Matrix4x4 viewMatrix_{};

    //正射影行列
    Matrix4x4 orthographicMatrix_{};

    //透視投影行列
    Matrix4x4 perspectiveFovMatrix_{};

    //ビューポート行列
    Matrix4x4 viewportMatrix_{};

public: // メンバ関数
    //コンストラクタ
    Camera();

    //デストラクタ
    ~Camera();

    //初期化
    void Initialize(int window_width = 1280, int window_height = 720);

    //更新
    void Update(const char* cameraName);

    //セッター

    /// <summary>
    /// translateの設定
    /// </summary>
    /// <param name="translate"></param>
    void SetTranslate(Vector3 translate) { this->translate_ = translate; }

    /// <summary>
    /// rotateの設定
    /// </summary>
    /// <param name="rotate"></param>
    void SetRotate(Vector3 rotate) { this->rotate_ = rotate; }

    void SetViewMatrix(Matrix4x4 viewMatrix) { this->viewMatrix_ = viewMatrix; }

    void SetPerspectiveFovMatrix(Matrix4x4 perspectiveFovMatrix) { this->perspectiveFovMatrix_ = perspectiveFovMatrix; }

    //ゲッター

    /// <summary>
    /// カメラの位置の取得
    /// </summary>
    Vector3 GetTranslate() const { return this->translate_; }

    /// <summary>
    /// カメラの回転角度の取得
    /// </summary>
    Vector3 GetRotate() const { return this->rotate_; }

    /// <summary>
    /// カメラ行列を取得する
    /// </summary>
    /// <returns></returns>
    Matrix4x4 GetCameraMatrix();

    /// <summary>
    /// ビュー行列の取得
    /// </summary>
    Matrix4x4 GetViewMatrix() const { return viewMatrix_; }

    /// <summary>
    /// 透視投影行列の取得
    /// </summary>
    Matrix4x4 GetPerspectiveFovMatrix() const { return perspectiveFovMatrix_; }

    /// <summary>
    /// 正射行列の取得
    /// </summary>
    Matrix4x4 GetOrthographicMatrix() const { return orthographicMatrix_; }

    /// <summary>
    /// ビューポート変換行列の取得
    /// </summary>
    Matrix4x4 GetViewportMatrix() const { return viewportMatrix_; }


    /// <summary>
    /// ワールド行列の作成
    /// </summary>
    void MakeWorldMatrix();

    /// <summary>
    /// ビュー行列の作成
    /// </summary>
    void MakeViewMatrix();

    /// <summary>
    /// 透視投影行列の更新
    /// </summary>
    void UpdatePerspectiveFovMatrix();

    /// <summary>
    /// 正射行列の更新
    /// </summary>
    void UpdateOrthographicMatrix();

    /// <summary>
    /// 透視投影行列の更新
    /// </summary>
    void UpdateViewportMatrix();

    /// <summary>
    /// 各行列の更新
    /// </summary>
    void UpdateMatrix();

    // 2Dで使うための現在のビューポートサイズ取得
    float GetViewportWidth() const { return width_; }
    float GetViewportHeight() const { return height_; }

};

