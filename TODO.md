# 今後の実装・拡張タスク (TODO / BACKLOG)

このファイルは、授業資料（`gakkousuraido` 等）に基づいた描画拡張タスクや機能実装のバックログについて、「**外部資料（画像等）を見なくても実装できるレベルの具体的な仕様**」をまとめたものです。

---

## 1. プリミティブと頂点フォーマットの拡張 (Ring等の高度な表現)

現在単色の形状となっているRingなどに、グラデーションやフェード（消失）処理を追加します。

### 1-1. `VertexData` 構造体への頂点カラーの追加
頂点ごとに独自の色と透明度を設定し、シェーダーで補間（グラデーション）させるための対応です。

- **C++側の変更 (`VertexData` の定義)**
  - `Vector4 color;` を追加します。
  - デフォルトは `{1.0f, 1.0f, 1.0f, 1.0f}` に初期化されるようにします。
- **パイプライン・レイアウト (`D3D12_INPUT_ELEMENT_DESC`)**
  - `{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }` を追加。
- **シェーダー側の変更 (HLSL)**
  - `VSInput` および `PSInput`（または `VertexShaderOutput`）構造体に `float4 color : COLOR;` を追加。
  - ピクセルシェーダーの出力計算で `textureColor * input.color * material.color` のように乗算を組み込みます。

### 1-2. Ringのカラー／アルファ値フェード実装
Ringの内側・外側で色を変えたり、円弧の開始位置から終了位置に向けて徐々に透明にする（軌跡のエフェクト等）ための仕様です。
- **`GenerateRingVertices` の引数拡張**
  - `Vector4 innerColor`, `Vector4 outerColor`
  - `float startAlpha`, `float endAlpha` （開始角度の透明度 と 終了角度の透明度）
- **アルファ値の計算アルゴリズム**
  - ループ内(`for (uint32_t i = 0; i < segments; ++i)`)で進行度を計算します。
    `float progress0 = static_cast<float>(i) / segments;   // 現在の頂点`
    `float progress1 = static_cast<float>(i + 1) / segments; // 次の頂点`
  - 線形補間（Lerp）を用いて各頂点のアルファ値を求めます。
    `float currentAlpha = std::lerp(startAlpha, endAlpha, progress0);`
  - `v0` (外側) には `Vector4(outerColor.x, outerColor.y, outerColor.z, currentAlpha)` を設定。
  - `v2` (内側) には `Vector4(innerColor.x, innerColor.y, innerColor.z, currentAlpha)` を設定。

### 1-3. 半径の動的制御（太さの可変制御）
剣の軌跡（剣閃エフェクト）などのように、先端が太く根本が細くなる、といった形状変化に対応させます。
- **引数の拡張**
  - `float startOuterRadius`, `float endOuterRadius`
  - `float startInnerRadius`, `float endInnerRadius`
- **半径の計算アルゴリズム**
  - アルファ値と同様に、セグメントの進行度合 `progress` に合わせて `std::lerp` で補間してそのセグメントの半径を決定します。
  - これにより、「外径は徐々に小さくなるが、内径は一定」といった動的なRing生成（三日月型など）が可能になります。

---

## 2. ParticleSystem のアーキテクチャ改良

通常のビルボード（カメラの方向を向く四角形ポリゴン）だけでなく、立体的な「Ring」等を生成し、エフェクトとしての表現力を強化します。

### 2-1. プリミティブの統合（メッシュパーティクル）
ParticleSystem で任意の `Object3DResource` （特にRingやCubeなど）を描画可能にします。
- **実装手順**:
  - `ParticleSystem` の初期化時に、描画したいメッシュタイプを指定できるようにする。
  - `PrimitiveManager::GetInstance()->GetStandardResource(PrimitiveType::Ring)` から参照を取得。
  - 描画命令(`DrawManager::DrawParticle`等)において、`IASetVertexBuffers` / `IASetIndexBuffer` に該当PrimitiveリソースのVBV/IBVをバインドし、`DrawIndexedInstanced` を呼び出す。
  - `Ring`などをShockwave（衝撃波）に使う場合、ビルボード（常にカメラを向く）とは異なり、ローカル空間のZ・Y軸などに沿って平置きで拡がるため、パーティクル側に**回転成分(Quaternion または Matrix)** および **3Dスケール成分(Vector3)** を持たせます。

### 2-2. パラメータアニメーションの拡充 (時間経過による変化)
パーティクルの生存時間（Lifetime）に応じた「動き」「色」「大きさ」の変化の自動化を行います。
- **Lifetimeごとの正規化計算**
  - 毎フレーム、`Particle` の `normalizedAge = age / lifeTime;` (値: `0.0f` 〜 `1.0f`) を計算。
- **補間（値のトランスフォーム）**
  - `normalizedAge` を用いて、`startScale` から `endScale` へ補間。
  - 単純な `std::lerp`（一定速度の変化）に加え、イージング関数（EaseOutCirc、EaseInBack など）を噛ませて緩急をつけるシステムを導入します。
    例: `float easeT = EaseOutCirc(normalizedAge);` -> `currentScale = std::lerp(startScale, endScale, easeT);`

### 2-3. UVスクロール (UV Scroll)
テクスチャのUV座標を時間経過でズラすことで、マグマ、雷、水面、エネルギー流などの表現を行います。
- **マテリアルデータへの追加**
  - `Material` 定数バッファ（あるいは パーティクル用の Instance データ）に `Matrix4x4 uvTransform` を追加。
- **C++側での計算**
  - UVスクロールの速度 `Vector2 uvScrollSpeed;` と オフセット `Vector2 uvOffset;` を定義。
  - 毎フレーム `uvOffset += uvScrollSpeed * deltaTime;`。
  - `uvTransform = Math::MakeAffineMatrix({1,1,1}, {0,0,0}, {uvOffset.x, uvOffset.y, 0});` を算出し定数バッファに転送。
- **HLSL シェーダー処理**
  - ピクセルシェーダー（または頂点シェーダー）にて:
    `float4 transformedUV = mul(float4(input.texcoord.x, input.texcoord.y, 0.0f, 1.0f), material.uvTransform);`
    `float4 texColor = tex.Sample(smpr, transformedUV.xy);`
