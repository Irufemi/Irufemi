#pragma once

#include "Core/Math/Vector3.h"
#include "Ball.h"

struct Spring{
    //アンカー。固定された端の位置
	Irufemi::Vector3 anchor{};
    //自然長
    float naturalLength{};
    //剛性。バネ定数k
    float stiffness{};
    //減衰係数
    float dampingCoefficient{};

    float deltaTime = 1.0f / 60.0f;

    //ボール
    Irufemi::Ball ball{};

};

