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

---

## 3. 【完了済みタスク記録】シーン遷移時の描画点滅（フリッカー）対策

### 3-1. 障害原因
シーン遷移時に `current_->Update()` がスキップされた際、描画ロジック（`Draw()`）は継続して走るため、未初期化のGPU定数バッファ（フレームインデックスが切り替わった先のバッファ）が参照され、未定義な座標や色で描画されてしまうことで、画面全体の激しい点滅や描画崩れが発生していました。

### 3-2. 対策方針と実装
- **GPUマルチバッファの初期化**: `ConstantBuffer` に、全フレームバッファへ同時にデータを転送する `UpdateAll()` メソッドを追加実装しました。各種Renderer系クラスに `isFirstUpdate_` フラグを導入し、リソース生成直後の初回更新時に限り、全バッファを一括で初期化する設計としました。
- **SceneManagerの遷移ロジック修正**: 根本原因として、シーンマネージャーが「フェードイン中（Opening）は Update() を呼ばずに Draw() だけを呼ぶ」仕様になっていたため、初回の Update が走る前に描画が行われていました。これにより、DrawManagerが前シーンのカメラ座標を参照し続けたり、オブジェクトが未初期化のゼロ行列で描画されて「画面自体が点滅する」現象が起きていました。これを防ぐため、`SceneManager.cpp` にて、ロード待機（LoadingWait）からフェードイン（Opening）に移行する瞬間に、**強制的に `current_->Update()` を1回だけ実行**する修正を加えました。これにより、描画が始まる前に全GPUバッファとカメラ設定が確実に初期化されるようになりました。

### 3-3. 今後のアーキテクチャの高度化（AAA級エンジンへの対応）
現状は「`Update()` 内でそのままGPUへの転送（Sync）を行っている」状態ですが、ゲーム制作の主流としては、以下の**責務分離**が推奨されます。
- **Updateフェーズ**: CPU上での論理的な座標計算・アニメーション計算のみを行う（GPUバッファへの転送はまだ行わない）。
- **PreDraw/Syncフェーズ**: 描画（コマンドリストへの積み込み）の直前に、全オブジェクトの最新のCPU計算結果をまとめてGPU定数バッファへ転送する。
これによって、シーンのフリーズやポーズ時などのエッジケースでも、意図しないフレーム不整合をより安全かつ一元的に防ぐことができます。段階的なリファクタリングを検討してください。

---

## 4. 【完了済みタスク記録】パーティクルシステムのカリング誤動作の対策

### 4-1. 障害原因
GameSceneにてプレイヤーが発射する弾の軌跡（`bulletTrail_`等）やミサイルの煙エフェクトが、特定のカメラの角度や位置において突然消えてしまう現象が発生していました。
原因は、軌跡エフェクトなどを描画する `ParticleSystem` の **視錐台カリング（Frustum Culling）が単一の基準座標に依存していたこと** です。
- カリングの判定基準（バウンディングスフィアの中心）が、パーティクルの初期発生座標（`emitter_.transform.translate`）で固定されていました。
- `PlayHitEffect(position, count)` などの座標指定発生メソッドは、引数の座標でパーティクルを生成するだけで、大元のエミッター座標を更新しません。
- その結果、弾が遠くに飛んでいって画面内にあっても、初期発生座標（大元のエミッター座標）が画面外に出た途端に、システム全体がカリングされて描画がスキップされていました。

### 4-2. 対策方針と実装
- 軌跡や飛び散るエフェクトのように、複数の発生地点が広範囲に及ぶパーティクルシステムについては、単一の基準点によるカリングは構造的に破綻するため、**該当のシステムに対しては明示的にカリングを無効化する (`SetCullingEnabled(false)`)** 対応を行いました。
- （対象: `bulletTrail_`, `missileFire_`, `missileSmoke_` 等）
- 今後、新たに武器の軌跡や広範囲に広がるエフェクトを追加する際も同様に、初期化時にカリング設定を必ず確認し、必要に応じて無効化する設計とします。
