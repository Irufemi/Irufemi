#include "DebugCamera.h"

void DebugCamera::Initialize(InputManager* input, int windowWidth, int windowHeight) {
    input_ = input;
    camera_.Initialize(windowWidth, windowHeight);
}

void DebugCamera::Update() {

    // カメラの現在のTransformを取得
    Vector3 translate = camera_.GetTranslate();

    //前移動の入力があったら
    if (input_->IsKeyDown('1') != 0) {
        const float speed = 1.0f;

        //カメラ移動ベクトル
        Vector3 move = { 0.0f,0.0f,speed };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }
    //後ろ移動の入力があったら
    else if (input_->IsKeyDown('2') != 0) {
        const float speed = -1.0f;

        //カメラ移動ベクトル
        Vector3 move = { 0.0f,0.0f,speed };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }
    //上移動の入力があったら
    if (input_->IsKeyDown('W') != 0) {
        const float speed = 0.01f;

        //カメラ移動ベクトル
        Vector3 move = { 0.0f,speed ,0.0f };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }
    //下移動の入力があったら
    else if (input_->IsKeyDown('S') != 0) {
        const float speed = -0.01f;

        //カメラ移動ベクトル
        Vector3 move = { 0.0f,speed,0.0f };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }
    //右移動の入力があったら
    if (input_->IsKeyDown('D') != 0) {
        const float speed = 0.01f;

        //カメラ移動ベクトル
        Vector3 move = { speed,0.0f,0.0f };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }
    //左移動の入力があったら
    else if (input_->IsKeyDown('A') != 0) {
        const float speed = -0.01f;

        //カメラ移動ベクトル
        Vector3 move = { speed,0.0f,0.0f };
        //移動ベクトルを角度分だけ回転させる

        //移動ベクトル分だけ座標を加算する
        translate = Math::Add(translate, move);
    }

    camera_.SetTranslate(translate);

    // 現在の回転角度を取得
    Vector3 rotate = camera_.GetRotate();

    // 回転速度
    const float rotationSpeed = 0.02f;

    //X軸回り回転の入力があったら
    if (input_->IsKeyDown('Z') != 0) {
        //X軸回りの角度を計算する
        rotate.x += rotationSpeed;

    }
    //反対方向
    else if (input_->IsKeyDown('X') != 0) {
        //X軸回りの角度を計算する
        rotate.x -= rotationSpeed;
    }
    //Y軸回り回転の入力があったら
    if (input_->IsKeyDown('C') != 0) {
        //Y軸回りの角度を計算する
        rotate.y += rotationSpeed;

    }
    //反対方向
    else if (input_->IsKeyDown('V') != 0) {
        //Y軸回りの角度を計算する
        rotate.y -= rotationSpeed;
    }
    //Z軸回り回転の入力があったら
    if (input_->IsKeyDown('B') != 0) {
        //Z軸回りの角度を計算する
        rotate.z += rotationSpeed;

    }
    //反対方向
    else if (input_->IsKeyDown('N') != 0) {
        //Z軸回りの角度を計算する
        rotate.z -= rotationSpeed;
    }

    // 更新した回転角度をカメラにセット
    camera_.SetRotate(rotate);

    //ImGuiでいじったり値が変更された部分
    camera_.Update("DebugCamera");
}