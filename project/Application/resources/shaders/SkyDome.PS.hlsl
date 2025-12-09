// SkyDome.PS.hlsl

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
    float3 n = normalize(input.normal);

    // ========= 1) 暗いベースカラー（夜・地下っぽい） =========
    float t = saturate(n.y * 0.5f + 0.5f); // 下0〜上1
    t = pow(t, 1.7); // 全体的に暗く

    float3 topColor = float3(0.02, 0.03, 0.06); // 上：紺
    float3 bottomColor = float3(0.06, 0.04, 0.06); // 下：少し紫
    float3 baseCol = lerp(bottomColor, topColor, t);

    // ========= 2) 月の方向を決める =========
    // 空のどこに月を出すか（ワールド空間の方向ベクトル）
    // 数値を変えると位置が変わる
    float3 moonDir = normalize(float3(0.3, 0.8, 0.4));

    // n と moonDir のなす角に応じて「月の中心からの距離」を決める
    float d = dot(n, moonDir); // 1.0 に近いほど月の中心

    // ========= 3) 月の本体（ディスク） =========
    // cos(角度) をしきい値にしてディスクを作るイメージ
    // 1.0 に近いほど中心、日本でざっくり radius 約5〜10度くらい
    float inner = 0.995; // 中心付近（濃い部分）
    float outer = 0.985; // 外周（ここより外はほぼ0）

    // smoothstep(edge0, edge1, x) → x<=edge0 で0, x>=edge1 で1
    // 中心(=d≈1)で1になるように outer→inner の順で使う
    float moonMask = smoothstep(outer, inner, d);

    // ========= 4) 月の周りのぼんやりしたハロー =========
    float haloInner = 0.985;
    float haloOuter = 0.95;
    float haloMask = smoothstep(haloOuter, haloInner, d);

    // ========= 5) 色を合成 =========
    float3 moonColor = float3(0.9, 0.9, 0.95); // ほぼ白
    float3 haloColor = float3(0.2, 0.25, 0.4); // 青白いハロー

    float3 col = baseCol;
    col += moonColor * moonMask * 1.8; // 本体は少し強め
    col += haloColor * haloMask * 0.7; // ハローは弱め

    return float4(saturate(col), 1.0f);
}
