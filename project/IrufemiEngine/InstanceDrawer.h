#pragma once

#include "math/shape/LinePrimitive.h"
#include <list>
#include <d3d12.h>

#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

// 前方宣言
class Camera;

class LineInstance {

public: //　メンバ関数(Scene内で描画時に呼ぶ関数)

    /// <summary>
    /// スクリーン(2次元)座標基準で線描画を行う関数
    /// </summary>
    /// <param name="startPos">始点</param>
    /// <param name="endPos">終点</param>
    /// <param name="color">色</param>
    /// <param name="camera">基準になるカメラ</param>
    void DrawLine2D(const Vector2& startPos, const Vector2& endPos, const Vector4& color, const Camera* camera);

    /// <summary>
    /// ワールド座標(3次元)基準で線描画を行う関数
    /// </summary>
    /// <param name="startPos">始点</param>
    /// <param name="endPos">終点</param>
    /// <param name="color">色</param>
    /// <param name="camera">基準になるカメラ</param>
    void DrawLine2D(const Vector3& startPos, const Vector3& endPos, const Vector4& color, const Camera* camera);


public: 
    // エンジン側で、Scene側でlistに積んだ分だけ描画を行う関数
    void Release();

    // エンジン側のフレーム開始時に前フレームで積んだ描画物をクリアする関数
    void Clear();

private:

    struct Instance {
        // 始点
        Vector4 origin;
        // 終点
        Vector4 end;
        // 色
        Vector4 color;
    };

    // 2Dのインスタンス。基本的にDepthは無効
    std::list<Instance> instance2D_;
    // 3Dのインスタンス。基本敵にDepthは有効。
    std::list<Instance> instance3D_;

    

};

