// MathUtility.hlsli

matrix MakeScaleMatrix(float3 scale) {
    return matrix(
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0,       1
    );
}

matrix MakeTranslateMatrix(float3 trans) {
    return matrix(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        trans.x, trans.y, trans.z, 1
    );
}

matrix MakeRotateXMatrix(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return matrix(
        1, 0,  0, 0,
        0, c,  s, 0,
        0, -s, c, 0,
        0, 0,  0, 1
    );
}

matrix MakeRotateYMatrix(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return matrix(
        c, 0, -s, 0,
        0, 1,  0, 0,
        s, 0,  c, 0,
        0, 0,  0, 1
    );
}

matrix MakeRotateZMatrix(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return matrix(
        c,  s, 0, 0,
        -s, c, 0, 0,
        0,  0, 1, 0,
        0,  0, 0, 1
    );
}

matrix MakeRotateXYZMatrix(float3 rot) {
    matrix rx = MakeRotateXMatrix(rot.x);
    matrix ry = MakeRotateYMatrix(rot.y);
    matrix rz = MakeRotateZMatrix(rot.z);
    return mul(mul(rx, ry), rz);
}

matrix MakeAffineMatrix(float3 scale, float3 rot, float3 trans) {
    matrix s = MakeScaleMatrix(scale);
    matrix r = MakeRotateXYZMatrix(rot);
    matrix t = MakeTranslateMatrix(trans);
    return mul(mul(s, r), t);
}

matrix MakeInverseTransposeMatrix(float3 scale, float3 rot) {
    float3 invScale = float3(
        scale.x != 0.0 ? 1.0 / scale.x : 0.0,
        scale.y != 0.0 ? 1.0 / scale.y : 0.0,
        scale.z != 0.0 ? 1.0 / scale.z : 0.0
    );
    matrix invS = MakeScaleMatrix(invScale);
    matrix r = MakeRotateXYZMatrix(rot);
    
    matrix invT = mul(invS, r);
    return invT;
}
