# IrufemiEngine 取扱説明書 (Manual)

## エディタ画面のレイアウトについて

エディタの画面構成（ドッキングウィンドウの配置など）が崩れてしまった場合や、チーム内で定められた最新の共通レイアウトに更新したい場合は、以下の手順で復元できます。

1. エディター画面上部のメニューバーから **`Window`** をクリック
2. **`Layout` -> `Load Default Layout`** をクリック

現在の自分の使いやすい配置をチームの新しいデフォルト設定にしたい場合は、並び替えたあとに **`Save Current as Default`** を押し、変更された `default_imgui.ini` をGitでコミットしてください。
（※初回クローン時は自動的に共通レイアウトが適用されるようになっています）

---

## GameObject とコンポーネントの基本操作

本エンジンは、Unityなどのモダンなエンジンと同様のコンポーネント指向で設計されています。シーン内のオブジェクト（`GameObject`）に対して、様々な機能（`Component`）をアタッチ・取得することでゲームロジックを構築します。

### 基本的なコンポーネントの取得
アタッチされているコンポーネントを取得するには、`GetComponent<T>()` を使用します。
```cpp
// 自身にアタッチされている TransformComponent を取得する
if (auto transform = gameObject_->GetComponent<TransformComponent>()) {
    transform->SetPosition(Vector3(0, 10, 0));
}
```

### GetComponentsInChildren を使った子孫の探索
`GameObject` 自身およびすべての子孫階層から特定のコンポーネントを一括検索する強力な機能が備わっています。特定のオブジェクト（プレハブのルートなど）の下に連なっているパーティクルやコライダーを一斉に操作したい場合に非常に便利です。

```cpp
auto obj = gameObject_->GetScene()->FindGameObject("Player");

// 1. ツリー全体から、指定した型のコンポーネントを「すべて（配列で）」取得する
auto emitters = obj->GetComponentsInChildren<ParticleEmitterComponent>();
for (auto pe : emitters) {
    pe->Restart(false); // 全てのエミッターを一斉に再発火
}

// 2. ツリー全体から、指定した型のコンポーネントを「1つだけ」取得する
if (auto collider = obj->GetComponentInChildren<ColliderComponent>()) {
    collider->SetIsActive(false);
}
```

**【重要】**
この探索機能は「自身（ルート）にアタッチされているコンポーネント」も検索対象に含まれます。そのため、ルートに目的のコンポーネントがあるかどうかを事前に気にする必要はなく、非常にシンプルで堅牢なコードを記述できます。

### コンポーネントのライフサイクル (OnEnable / OnDisable)
コンポーネントには初期化(`Initialize`)や毎フレームの更新(`Update`)に加えて、オブジェクトが有効化・無効化されたタイミングで呼ばれるフックが用意されています。

- **`OnEnable()`**: `GameObject` の `SetIsActive(true)` が呼ばれた際や、アクティブな状態でコンポーネントがアタッチされた直後に呼ばれます。オブジェクトプールから復帰した際の状態リセットや、イベントの登録に最適です。
- **`OnDisable()`**: `GameObject` の `SetIsActive(false)` が呼ばれた際や、破棄される直前に呼ばれます。イベントの解除などに使用します。

---

## チーム開発ルール・コーディング規約

チームでの共同開発（`Application_team` など）を進めるにあたり、以下のアーキテクチャ・コーディング規約を遵守してください。

### 1. アーキテクチャと関心の分離
- **エンジンの独立性**: `IrufemiEngine/` フォルダ配下のコア機能には、特定のゲームやシーンに依存する処理・固有のデータ・アクターを**絶対に含めない**でください。
- **ゲームロジックの配置**: ゲーム固有のロジックやキャラクター制御は、必ず `Application_team/` (または各ゲームの Application フォルダ) 内に記述し、エンジンとアプリケーションの境界を厳格に保ちます。

### 2. メモリ管理と安全性 (C++ / DirectX)
- **スマートポインタの利用**: メモリリークを防ぐため、生ポインタ(`Raw Pointer`)の新規使用は極力避け、用途に合わせて `std::unique_ptr` や `std::shared_ptr` を優先してください。
- **COMオブジェクト管理**: DirectXのオブジェクトを扱う際は、必ず `Microsoft::WRL::ComPtr` を使用して安全にライフサイクルを管理してください。
- **エラーチェックとロギング**: DirectXのAPI呼び出し時やJSON等のパース時は必ずエラーチェックを行い、エラーメッセージの出力には `printf` や `std::cout` ではなく、エンジン標準の `Log::OutPutLog` を使用してください。
  ```cpp
  #include "Engine/Core/Utility/Log.h"
  // エディタのコンソールウィンドウ等にも出力されるように、エンジン指定のロガーを使用する
  Log::OutPutLog(std::cerr, "Error: Failed to load file.\n");
  ```

### 3. オブジェクトのライフサイクル管理とプールの安全な運用
シューティングゲームの敵やヒットエフェクトなど、頻繁に生成と消滅を繰り返すオブジェクト（プーリング対象）を実装する際は、メモリ破壊やプールの崩壊を防ぐために以下の**厳密なルール**に従う必要があります。

本エンジンでは、AAA基準の完全なゼロ・アロケーションを実現するため、オブジェクトプール (`ObjectPool<T>`) はポインタではなく **`Handle` (整理券)** による管理へ移行しました。

- **プールへの返却は必ず `Release(handle)` を使用する**
  プーリング対象のオブジェクトが死ぬとき（非アクティブ化されるとき）は、自身が生成時に受け取った `Handle` を用いてプールへ返却してください。`Destroy()` を呼ぶとメモリから消去されプールが崩壊します。
  ```cpp
  // 敵の生成時に Handle をマップなどに記憶しておく
  activeEnemyHandles_[enemy.get()] = handle;

  enemyComp->SetOnDeathCallback([this](GameObject* deadObj) {
      deadObj->SetIsActive(false);
      if (enemyPool_) {
          auto it = activeEnemyHandles_.find(deadObj);
          if (it != activeEnemyHandles_.end()) {
              enemyPool_->Release(it->second); // Handle を使って最速で返却
              activeEnemyHandles_.erase(it);
          }
      }
  });
  ```

- **【重要】Handleの安全性 (Generation) とエディタ連携 (EditorMode)**
  新しい Handle には **`generation` (世代)** という概念が組み込まれています。古い Handle（すでに返却済みのもの）を使って `Resolve(handle)` を呼び出しても、世代が不一致となるため自動的に弾かれて `nullptr` が返ります（ダングリングポインタの完全な防御）。
  また、デバッグビルド（`EditorMode` 等）の時は、Handle 構造体の中に `debugPtr` という実体へのポインタが自動で含まれます。これにより、エディタ（ImGui）のインスペクタ上で「ただの数字の羅列」ではなく、実際のオブジェクトの名前（命名）を確認しながらデバッグを行うことができます。

- **LifetimeComponent の TimeoutAction を活用する**
  一定時間で消滅するエフェクトなどに `LifetimeComponent` をアタッチする場合、インスペクタ上で **Timeout Action** を変更できます。プール運用なら必ず `Disable` を設定してください。コードから生成する際は以下のように上書きすると安全です。
  ```cpp
  auto obj = gameObject_->Instantiate(effectPrefabPath);
  if (auto lifetime = obj->GetComponent<LifetimeComponent>()) {
      lifetime->SetTimeoutAction(TimeoutAction::Disable);
  }
  ```

- **【重要】Updateループ中の遅延削除 (Deferred Deletion / Pending Kill)**
  コンポーネントの `Update()` 処理中に、自分自身や子オブジェクトを即座にツリーから引き剥がす (`RemoveChild` や `ReleaseGameObject` など) 操作を行うと、ループ処理中の親の `children_` 配列のイテレータが無効化され、**アクセス違反（クラッシュ）**の原因となります。
  これを防ぐため、マネージャーによる **遅延削除キュー (Pending Kill)** を必ず実装してください。
  1. マネージャークラスに `std::vector<std::shared_ptr<GameObject>> pendingReleases_;` を定義する。
  2. 削除を要請する側は `manager_->MarkForRelease(this_obj)` を呼ぶだけにする（この時点ではまだ消えない）。
  3. マネージャーの `Update()` の最後（すべてのオブジェクトのUpdateが完了した安全なタイミング）で、キューに溜まったオブジェクトを一気にプールへ返却し、キューをクリアする。

### 4. GPUリソースの事前確保（Pre-warming）によるラグ防止
リアルタイム性が命のゲームにおいて、実行中のテクスチャ読み込みやシェーダー・VRAMバッファの構築（遅延評価）は**画面のカクつき（Hitch / Stutter）を生む最大の原因**となります。一線級のエンジンと同様に、ゲーム開始前の初期化フェーズ（ロード画面や `Start()` のタイミング）で重い処理を強制的に終わらせる**プレウォーム（事前確保）**を徹底してください。

- **安全なプレウォームの実装方法**
  初期化時にフルセットの更新処理を呼ぶと、プールの中で休眠中のエフェクトが勝手に発生・消費されてしまう（いざ使うときに出なくなる）バグの原因になります。これを防ぐため、**「発生パラメータは一切送らず、GPUマネージャーに対してハンドルの取得（VRAM領域の予約）だけを行う」専用のメソッド**を用意・使用してください。
  ```cpp
  // ParticleObject.cpp の例
  void ParticleObject::Initialize() {
      // ...
      // 実行中のラグを防ぐため、安全なプレウォーム（枠だけの事前確保）を行う
      PrewarmSystem();
  }

  void ParticleObject::PrewarmSystem() {
      // パラメータは送信せず、マネージャーに「このテクスチャとブレンドモードを使う」と登録だけ行う
      if (!emitterHandle_.IsValid() && gpuParticleManager_) {
          emitterHandle_ = gpuParticleManager_->RegisterEmitter(texturePath_, blendMode_, isUnscaledTime_);
      }
  }
  ```

### 5. パフォーマンスと Data-Oriented な設計 (ターゲット管理)
シーン内の特定のオブジェクト（敵やボスなど）を毎フレーム再帰的に検索する処理は、オブジェクト数が増えるにつれて急激なCPU負荷（O(N)問題）を引き起こします。
これを防ぐため、ロックオン対象などの特定の性質を持つオブジェクトの管理には、**自己登録型コンポーネント（例: `TargetableComponent`）** を使用してください。

- **`TargetableComponent` の利用**:
  対象オブジェクトの生成時に `TargetableComponent` をアタッチしておくと、`OnEnable` 時に対象がグローバルな静的リストへ自動登録されます。
  検索側は `TargetableComponent::GetTargets()` をループで回すだけになり、定数時間かつキャッシュ効率の良いアクセスが可能になります。
  ※ `TargetableComponent` はエンジンコアの機能ではなく、`Application` 側に実装されるアーキテクチャパターンの例です。

### 6. 非同期レイキャスト (Async Raycast) と物理クエリの最適化
毎フレーム大量のオブジェクトに対して同期的にレイキャスト（視線判定など）を行うと、メインスレッドの処理落ち（フレームドロップ）の大きな原因となります。
これを防ぐため、`CollisionManager` にはスレッドプールを利用した非同期レイキャストAPI `RaycastAsync` が用意されています。

- **非同期クエリの発行と Amortization (分散処理)**
  毎フレーム全ての判定を行うのではなく、`std::future` を用いてバックグラウンドで処理させ、結果が出たタイミングでキャッシュを更新する **Time-Slicing** の設計を強く推奨します。

  ```cpp
  // 1. ヘッダ側で future と結果を保持するキャッシュ変数を用意
  #include <future>
  struct TargetVisibilityCache {
      bool canSee = true;
      float lastCheckTime = -1.0f;
      std::shared_ptr<std::future<std::pair<bool, RaycastHit>>> pendingTask;
  };
  std::unordered_map<GameObject*, TargetVisibilityCache> visibilityCache_;

  // 2. 実装側 (Update)
  float currentTime = engine->GetTotalTime();

  // 既に投げている非同期判定の終了をポーリング (ノンブロッキング)
  for (auto& [objPtr, cache] : visibilityCache_) {
      if (cache.pendingTask && cache.pendingTask->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          auto result = cache.pendingTask->get();
          bool hit = result.first;
          RaycastHit hitInfo = result.second;
          
          // 判定結果をキャッシュに保存
          cache.canSee = (hit && hitInfo.hitObject == objPtr); // 簡易例
          cache.pendingTask.reset();
      }
  }

  // ターゲットへの判定発行（例: 0.1秒間隔に分散）
  if (currentTime - cache.lastCheckTime > 0.1f && !cache.pendingTask) {
      cache.lastCheckTime = currentTime;
      Ray ray = { cameraPos, dir };
      cache.pendingTask = std::make_shared<std::future<std::pair<bool, RaycastHit>>>(
          engine->GetCollisionManager()->RaycastAsync(engine->GetThreadPool(), ray, maxDistance, layerMask)
      );
  }
  ```

### 7. 命名規則・コードスタイル
- **メンバ変数の命名**: `m_` などの接頭辞は使用せず、**キャメルケースの末尾にアンダーバー**をつけるスタイル (`variableName_`) に統一してください。
- **ヘッダーの注釈**: 関数やクラスのコメントは「Doxygen形式」で記述してください。
- **インクルードガード**: `#pragma once` を使用してください。
- **既存への適応**: 新しくクラスや関数を追加する際は、必ず周囲の「既存のコードベースの命名規則」に合わせ、自己流のスタイルを混入させないでください。
- **文字コードとフォーマット**: ファイルはすべて `UTF-8 (署名なし)` で保存し、`.clang-format` による自動整形を活用してください。

---

## 静的モデルの描画 (MeshRendererComponent)

アニメーションを持たない背景モデルやプロップを描画する場合は `MeshRendererComponent` を使用します。

### 基本的な使い方
`GameObject` に `MeshRendererComponent` をアタッチし、描画したいモデル（OBJ / GLTF / FBXなど）をセットします。

```cpp
auto renderer = gameObject_->AddComponent<MeshRendererComponent>();
renderer->LoadModel("sample/cube.gltf");
```

※ アニメーションを行わないため、後述の `SkinnedMeshRendererComponent` よりも軽量に動作します。動かない物体にはこちらを優先して使用してください。

### 3Dプリミティブの描画 (PrimitiveRendererComponent)
テスト用の床や障害物など、モデルファイルを用意せずに簡易的な立体を描画したい場合は `PrimitiveRendererComponent` を使用します。

```cpp
auto primitive = gameObject_->AddComponent<PrimitiveRendererComponent>();
primitive->SetShape(PrimitiveType::Cube); // Cube, Sphere, Cylinder, Cone, Torus など
primitive->SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色
```

インスペクタ上からも形状の変更や、材質パラメータ（Roughness / Metallic）の調整が可能です。

---

## アニメーションモデルとデバッグ機能の利用方法

本エンジンでは、アニメーションする3Dモデルを描画するための `SkinnedMeshRendererComponent` と、アニメーションの再生ロジックを管理する `AnimatorComponent` の**二段構え（分業）アーキテクチャ**を採用しています。

### 基本的な使い方
1. GameObjectに `SkinnedMeshRendererComponent` をアタッチし、描画したいモデル（GLTF等）をセットします。
2. 同一のGameObjectに `AnimatorComponent` をアタッチし、再生したいアニメーションファイルをセットします。
3. `AnimatorComponent` 側で再生（Play）指示を出すと、自動的に `SkinnedMeshRendererComponent` へ姿勢データが転送され、GPU Skinningによって高速に描画されます。

### ボーンのデバッグ可視化 (X-Ray描画)
アニメーションのモーション確認や、武器の取り付け位置の確認などに、キャラクターの骨格（ボーン）を画面上に直接描画してデバッグすることができます。

1. **全体表示の切り替え**
   エディタの `Camera & Lights` タブ（DebugScene等）内にある **`Show All Debug Bones`** チェックボックスをONにします。
   これにより、シーン内のすべてのアニメーションモデルのボーンが一斉に可視化されます。
   
2. **描画の仕様 (X-Ray表示)**
   - **関節 (Joints)**: スカイブルーの「球（Sphere）」で描画されます。
   - **骨 (Bones)**: ピンク色の「八面体（Octahedron）」で描画されます。
   - 深度テストが無効（X-Ray表示）になっているため、キャラクターのメッシュに隠れることなく、常に最前面に現在のボーンの姿勢が描画されます。

### 個別オブジェクトの骨格デバッグ (SkeletonDebugRendererComponent)
シーン全体ではなく、特定のキャラクターのみの骨格を確認したい場合や、ボーンの「ローカル軸の向き（X, Y, Z軸）」を個別に可視化したい場合は、対象の GameObject に `SkeletonDebugRendererComponent` をアタッチしてください。
（※ 同一の GameObject に `SkinnedMeshRendererComponent` がアタッチされている必要があります）

### デバッグカメラのシームレスな移行
エディタの `Camera & Lights` タブにある `Debug Camera Mode` にチェックを入れると、ゲームカメラからデバッグカメラ（自由操作）に切り替わります。
この際、カメラの位置が初期化されて画面が飛ぶことはなく、**「現在見ているメインカメラの座標と回転」を自動的に引き継いで（スナップして）スタートする** 仕様になっているため、気になった箇所から即座にデバッグ作業へ移行できます。

---

## パーティクルシステム (GPUParticleSystem) の利用方法

本エンジンのパーティクルシステムは、コンピュートシェーダー(CS)によってGPU上で高速に動作します。
スクリプトやコンポーネントから以下の手順でエミッターを追加・操作することができます。

### コンポーネントからの利用
GameObject に `ParticleEmitterComponent` をアタッチするだけで、自動的にエディタ上で操作・プレビューが可能です。
また、より高度な表現として、ベクトル場を用いた `ParticleFieldComponent` や、ボクセルベースの `VoxelParticleComponent` などの拡張コンポーネントも提供されています。
エディタ（ImGui）上で設定したパラメータは、JSONファイルとして自動的にシリアライズされ、再実行時にも完全に復元されます。

### プログラムからの直接利用 (ParticleObject)
ゲーム内でコードから動的にパーティクルを生成・制御したい場合は `ParticleObject` クラスを使用します。
JSONファイルから設定をロードすることで、エディタで作成した複雑なエフェクトをそのまま呼び出すことができます。

```cpp
#include "Renderer/Object/Particle/ParticleObject.h"

// 1. ParticleObject の生成とJSONの読み込み
ParticleObject myParticle;
myParticle.LoadFromJson("resources/particles/explosion.json");

// 2. 座標や必要に応じたパラメータの上書き
myParticle.position_ = Vector3(10.0f, 5.0f, 0.0f);
myParticle.emissionRate_ = 100.0f; // 1秒間に100個発生

// （パラメータをコードから変更した場合は MarkDirty() を呼ぶか、Update内で自動反映されます）
myParticle.MarkDirty();

// 3. 毎フレーム Update を呼ぶ
myParticle.Update();

// 4. 一度に大量に発生（バースト）させたい場合
myParticle.EmitBurst(50);
```

### 【上級者向け】GPUParticleManager の直接利用
直接マネージャーに通信して描画リクエストを送ることも可能です（独自の最適化を行いたい場合など）。
※現在のマネージャーは「テクスチャ + ブレンドモード + タイムスケール」の複合キーで管理されています。

```cpp
#include "Renderer/System/ParticleGPU/GPUParticleManager.h"
#include "Renderer/System/ParticleGPU/GPUParticleSystem.h"

// 1. マネージャーにエミッターを登録（テクスチャ、ブレンドモード、ポーズ中動作フラグ）
auto handle = GPUParticleManager::GetInstance()->RegisterEmitter(
    "effect/particle_tex.png", 
    BlendMode::kBlendModeAdd, 
    false // trueにするとポーズ中(UnscaledTime)でも動作する
);

// 2. パラメータを設定してマネージャーに更新を通知
GPUParticleEmitter data;
data.emit = 1;
data.type = 0; // 0: Sphere, 1: Beam, 2: Ring, 3: Cylinder, 4: Box
data.translateX = 10.0f;
data.emissionRate = 50.0f; // 1秒あたりの連続放出数

// 3. データの適用
GPUParticleManager::GetInstance()->UpdateEmitterData(handle, data);
```

### GPU Bitonic Sort による半透明ソート (Zソート)
本エンジンのGPUParticleSystemは、単に更新処理をGPUで行うだけでなく、**カメラからの距離に応じた半透明描画の並び替え（Zソート）も、完全にGPU上のCompute Shader（ビトニックソート）で完結**しています。
これにより、CPUへデータを差し戻すオーバーヘッドをゼロにしつつ、数万のパーティクルが奥から手前へ正しくアルファブレンドされるAAA品質のレンダリングパイプラインが構築されています。開発者はソートの負荷や描画順序の破綻を一切気にする必要がありません。

### ゲーム中での一時的なエフェクト再生 (爆発など)
シーン内の特定座標に単発（ワンショット）の爆発エフェクトなどを出したい場合は、新しく追加された `Effect` クラスを使用するのが最も簡単です。

```cpp
#include "Renderer/Object/Effect/Effect.h"

// 1. エフェクトインスタンスの作成と初期化（例：爆発）
Effect myEffect;
myEffect.Initialize(EffectType::kExplosion);

// 2. 指定した座標でエフェクトを発生させる
myEffect.Play(Vector3(10.0f, 0.0f, 5.0f));

// 3. 毎フレーム Update と Draw を呼ぶ
myEffect.Update();
myEffect.SyncBeforeDraw();
myEffect.Draw();
```

### インスペクターからの ParticleType などの設定
`ParticleEmitterComponent` を GameObject にアタッチした場合、エディターの **Inspector パネル** から以下の新機能を直感的に操作できます。

- **Particle Mesh & Shape (形状と発生範囲)**
  - `Sphere`, `Beam`, `Ring`, `Cylinder`, `Box` などの発生形状を選択可能です。
  - `Box` を選択した場合のみ、専用の `Area Size (X,Y,Z)` を指定して箱状の範囲内に発生させることができます。
  - **Billboard Mode**: パーティクルのカメラに対する向きを `None` (固定), `Billboard` (常にカメラを向く), `Y-Axis` (Y軸固定でカメラを向く・魔法陣などに最適) から選べます。

- **Animation & Visuals (アニメーションと見た目)**
  - **Atlas Rows / Cols**: 連番テクスチャ（スプライトシート）の分割数を指定するだけで、自動的にアニメーション再生されます。
  - **Start / Mid / End Color & Scale**: これまでの開始/終了だけでなく、「中間色・中間スケール」と「それがどのタイミング(Mid Point)で切り替わるか」を設定でき、爆発（白→オレンジ→黒煙）などの複雑な表現が可能になりました。

- **Physics (物理挙動)**
  - 重力やバウンドに加えて、**Jitter (ジッター)** によって不規則なブレ（ノイズ）を与え、魔法の粉や舞い散る火の粉のようなランダムな動きを表現できます。

【重要】これらのパラメータはすべて Inspector のGUIからリアルタイムに変更・確認できます。
本エンジンでは `ParticleEmitterComponent` が `CanUpdateInEditMode()` をサポートしているため、**ゲームを再生していなくても（エディタ編集モードでも）、パラメータを変更した瞬間にリアルタイムでパーティクルの見た目が更新・プレビューされます。**

### 【開発者向け】パラメータ追加・UI連携の仕組み (RegisterProperties)
`ParticleObject` が持つパラメータ群をインスペクターに表示するためのUI登録処理は、すべて `ParticleObject::RegisterProperties(Component* comp)` というメソッドに集約されています。
これにより、エンジンコアの描画処理とエディターUIの責務が完全に分離されました。もし新しい機能やパラメータを追加したい場合は、この `RegisterProperties` 内に `comp->RegisterProperty(...)` などを1行追記するだけで、自動的にエディタUIに項目が追加され、保存（シリアライズ）も連動して行われるようになっています。

---

## ポストプロセス (PostProcessManager) の利用方法と描画順序

画面全体にかけるポストプロセスエフェクト（PostProcessManager）を使用する際は、**「エフェクトをスタックに追加する順番（描画順序）」** を意識することで、プロの現場でも通用する意図した映像表現が可能になります。

### 推奨される描画順序（スタックに追加する順）
1. **色調補正系**: ToneMapping, Grayscale, Sepia, HSV など
2. **空間・ぼかし系**: Smoothing, GaussianFilter, RadialBlur など
3. **画面演出系**: Vignette, Noise, Glitch, Dissolve など
4. **画面遷移系**: Fade, Slide など

**なぜこの順番なのか？**
例えば、`Vignette`（画面の端を暗くする/色をつける演出）のあとに `Grayscale`（白黒化）をかけてしまうと、ビネットで赤色などを指定してもモノクロになってしまいます。「色調補正」を先に行い、その上から「画面演出」を乗せるのがセオリーです。

### レイヤー機能 (PreUI / PostUI)
各エフェクトはスタックに追加する際、適用レイヤーを指定することができます。
- **`EffectLayer::PreUI`**: 3Dシーンや背景にのみ適用され、UI（ImGuiやCanvas等）には影響を与えません（デフォルト）。
- **`EffectLayer::PostUI`**: 最終的なUI描画もすべて完了した後に、画面全体に対して適用されます。

### 多彩なエフェクト・新機能
VignetteやNoise等の基本機能に加え、AAAタイトル級の様々なエフェクトが実装されています。
- **DualKawaseBlur**: 従来のGaussianよりも広範囲かつ低負荷にぼかしをかけることが可能です。
- **DepthOfField**: ピントの距離(FocusDistance)を指定し、前後の風景をぼかす被写界深度エフェクトです。
- **LightShafts**: 光源のスクリーン座標を指定し、オブジェクトの隙間から漏れる光の筋（ゴッドレイ）を描画します。
- **その他**: `ChromaticAberration` (色収差)、`DisplacementMap` (陽炎・歪み)、`Pointillism` (点描画)、`NightVision` (暗視ゴーグル) など多数のモードが利用可能です。

### Vignetteのパラメータ変更について
Vignetteエフェクトがより自然な減衰（Smooth Falloff）になるようパラメータがアップグレードされました。
- **`radius` (旧: scale)**: 減衰が始まる半径 (デフォルト 0.8)
- **`softness` (旧: power)**: 減衰の柔らかさ (デフォルト 0.5)

これにより、画面端が完全に黒く潰れるのを防ぎ、滑らかなグラデーション表現が可能になっています。シーン初期化時などでパラメータを調整する際はご留意ください。

---

## オブジェクト個別のカスタムエフェクト適用

画面全体にかける通常のポストプロセスとは異なり、特定のキャラクターがダメージを受けたときの点滅や、敵が倒れたときのディゾルブ（消失）など、**特定のオブジェクトに対してのみエフェクトを適用**するための機能です。

### 1. カスタムパラメータの登録方法
エフェクトのパラメータ（しきい値や色など）は、GPUに送る前に `PostProcessManager` に登録してID（`effectParam`）を取得する設計になっています。

```cpp
auto* ppm = engine->GetPostProcessManager();
PostProcessManager::CustomEffectParams params;
params.param1 = 0.5f; // 例: ディゾルブのしきい値など
uint32_t id = ppm->RegisterCustomEffectParams(params);
float effectParam = static_cast<float>(id) / 255.0f;
```

### 2. 個別適用可能なエフェクトの種類 (PostProcessMode)
用意されている各種エフェクト（PostProcessMode列挙体）のうち、個別適用に使いやすい代表的なものを紹介します。
- **Dissolve (8)**: ノイズテクスチャによる消失演出。敵を倒した際などに。
- **Glitch (15)**: ノイズ・色収差による映像の乱れ。ダメージ時の点滅や異常状態に。
- **LuminanceBasedOutline (17)**: 輝度ベースのアウトライン。キャラクターがダメージを受けたときの縁取り発光などに。
- **Pixelation (18)**: ドット絵化（モザイク）。
- **Halftone (26)**: 網点・コミック調。
- **Fade (12) / Slide (13)**: ワイプや指定色へのフェード。
（※その他の全エフェクトの一覧も `PostProcessManager.h` に記載されています）

### 3. 動的オブジェクトへの適用 (EffectMaskComponent)
キャラクターやボスなど、個別の `GameObject` にアタッチしてエフェクトを管理する方法です。
時間経過で自動的にエフェクトが切れる `duration` 管理機能が内蔵されています。

```cpp
// 使い方
gameObject->AddComponent<EffectMaskComponent>();

// 発動時 (effectType, effectParam, duration(秒))
gameObject->GetComponent<EffectMaskComponent>()->ApplyEffect(8, effectParam, 1.0f);
```

### 4. バッチ描画（環境物）への適用 (ModelBatchRendererComponent)
大量に配置された静的・環境オブジェクトの、**特定のインスタンスにのみ** エフェクトを指定する方法です。
ドローコールを1回に抑えたまま（Instancing）、インスタンス個別にエフェクトを描画できるため、**パフォーマンスに非常に優れたアプローチ**です。

```cpp
// 1個目は通常描画（エフェクトなし）
batchComponent->AddInstance(transform1);

// 2個目には個別のエフェクト（例: Type 8 = Dissolve）を適用
batchComponent->AddInstance(transform2, 8, effectParam, true);
```

---

## Audio システム (AudioPlayer / AudioSourceComponent) の利用方法

本エンジンのAudioシステムは、コンポーネントからの利用とプログラムからの直接利用の2通りの方法をサポートしています。
これまでの `Bgm` や `Se` クラスは廃止され、統合された `AudioPlayer` クラスによって一元管理されます。リソースリークを防ぐ安全な設計（ComPtrやRAIIの活用）が内部で行われているため、プログラマは生成・破棄のタイミングを気にせず利用できます。

### コンポーネントからの利用
GameObject に `AudioSourceComponent` をアタッチすることで、インスペクタ（エディタ）上からサウンドの設定が可能です。
- **Audio Type**: `BGM` か `SE` かを選択します。BGMはデフォルトでループ再生されます。
- **File Path**: 再生するオーディオファイルのパス（例: `resources/audio/bgm/field.wav`）を指定します。
- **Volume**: 音量を 0.0 ～ 1.0 の間で調整します。
- **Loop**: ループ再生のON/OFFを任意に切り替えます。

スクリプトから再生・停止を行う場合は、コンポーネントを取得して以下のように呼び出します。
```cpp
auto audioSource = GetComponent<AudioSourceComponent>();
if (audioSource) {
    audioSource->Play(); // 再生（インスペクタの設定が反映されます）
    // audioSource->Stop(); // 停止したい場合
}
```

### プログラムからの直接利用 (AudioPlayer)
UIの操作音や、特定のコンポーネント（GameObject）に紐付かない効果音を再生する場合は、直接 `AudioPlayer` クラスのインスタンスを生成して使用するのが便利です。

```cpp
#include "Resource/Audio/AudioPlayer.h"
#include "Resource/Audio/AudioType.h"

// 1. エンジンから AudioManager を取得して、AudioPlayer を生成
auto audioManager = engine->GetAudioManager();
AudioPlayer clickSe(audioManager, AudioType::SE);

// 2. 音声ファイルのパスを指定して初期化
clickSe.Initialize("resources/audio/se/click.wav");

// 3. 再生（オプションで音量設定などが可能）
clickSe.SetVolume(0.8f);
clickSe.Play(); // 1回再生

// 再生中のサウンドを明示的に止めたい場合は Stop() を呼びます
// clickSe.Stop();
```

---

## アクションベース入力システム (Input Action System) の利用方法

現在の入力システムは、キーボードやゲームパッドのボタンを直接監視するのではなく、物理入力と「論理アクション（例：Jump、Move）」を紐付けて管理する「アクションベース」へ移行しています。これにより、ユーザーのキーコンフィグの変更や複数デバイスの同時対応が容易になります。

### 使い方（初期化時）
ゲームの初期化処理（Sceneの `Initialize` など）で、`InputManager` に対してアクションと物理デバイス（キーやボタン）の対応付け（バインディング）を行います。

```cpp
InputManager* input = engine_->GetInputManager();

// "Jump" アクションに Spaceキー と ゲームパッドのAボタン を割り当て
input->BindAction("Jump", InputId::Keyboard_Space);
input->BindAction("Jump", InputId::GamePad_A);

// "MoveX" アクション（アナログ/1D軸入力）の割り当て
// スケール値（第3引数）を使って、物理入力を最終的なアクション値に変換します。
// 例: Dキー(1.0)は右方向(+1.0)に、Aキー(1.0)はマイナスを掛けて左方向(-1.0)にする
input->BindAction("MoveX", InputId::Keyboard_D, 1.0f);
input->BindAction("MoveX", InputId::Keyboard_A, -1.0f);
// パッドのスティックはそのままの値(-1.0～1.0)を使う、あるいは 0.5f 等を掛けて感度調整も可能
input->BindAction("MoveX", InputId::GamePad_LeftStickX, 1.0f);
```

### 使い方（更新時）
毎フレームの更新処理（Sceneの `Update` など）では、バインドしたアクション名を指定して状態を取得します。

```cpp
InputManager* input = engine_->GetInputManager();

// デジタル入力（ボタンが押された瞬間）の判定
if (input->IsActionTriggered("Jump")) {
    // ジャンプ処理を実行
}

// アナログ入力（移動量など）の取得
float moveInputX = input->GetActionValue("MoveX").x;
// プレイヤーの移動処理へ
```

※ 互換性維持のため、従来の `IsKeyDown(VK_SPACE)` などのAPIも引き続き使用可能ですが、新しくコードを書く際はアクションシステム (`BindAction`, `GetActionValue` 等) を積極的に利用することが推奨されます。

---

---

## リソース管理システム (ResourceHandle) の利用方法

本エンジンでは、AAA規模の商用エンジン（メモリ予算の厳格な管理やLRUパージ機構）を見据え、テクスチャやモデルデータなどの巨大なリソースの管理を `std::shared_ptr` から独自の **`ResourceHandle` ベースのアーキテクチャ** へと完全移行しました。

これにより、不用意な `shared_ptr` の循環参照によるメモリリークや、解放タイミングの制御不能といった問題を解決し、高速かつ安全なリソース参照が可能になっています。

### コンポーネント開発時におけるリソースの持ち方
自作のコンポーネント（RendererやEffectなど）でテクスチャやモデルを保持する場合、これまでのように `std::shared_ptr<ManagedModel>` をメンバ変数に持つことは禁止されています。代わりに `ResourceHandle` を保持してください。

```cpp
#include "Engine/Core/System/ResourceCachePool.h" // ResourceHandle用

class MyCustomRenderer : public Component {
private:
    ResourceHandle myModelHandle_;
    ResourceHandle myTextureHandle_;
};
```

### リソースのロード（取得）と解放
リソースのロードは、各Manager（`TextureManager`, `ModelManager` など）の `Load***` メソッドを使用します。**取得したハンドルは、コンポーネント破棄時に必ず手動で `Release***` を呼んで解放（参照カウントを下げる）してください。**

```cpp
void MyCustomRenderer::Initialize() {
    // リソースをロードしてハンドルを保持
    myModelHandle_ = engine_->GetObjModelManager()->LoadModel("EnemyModel");
    myTextureHandle_ = engine_->GetTextureManager()->LoadTexture("EnemyTex");
}

void MyCustomRenderer::Finalize() {
    // 破棄時にハンドルの参照カウントを減らす
    if (myModelHandle_.IsValid()) {
        engine_->GetObjModelManager()->ReleaseModel(myModelHandle_);
    }
    if (myTextureHandle_.IsValid()) {
        engine_->GetTextureManager()->ReleaseTexture(myTextureHandle_);
    }
}
```

### 毎フレームの描画（ハンドルの解決）
描画や更新（Update / Draw）のタイミングで初めて、ハンドルから実際の生ポインタ（実データ）を **解決 (Resolve)** します。
※解決した生ポインタはメンバ変数に保持せず、その関数（スコープ）内だけで使い捨ててください。万が一リソースが裏でパージ（破棄）されても、安全にフォールバック（ダミー白テクスチャなど）が返る仕組みになっています。

```cpp
void MyCustomRenderer::Draw() {
    // ハンドルから実体を解決（ポインタ取得）
    auto model = engine_->GetObjModelManager()->Resolve(myModelHandle_);
    
    // データがまだロードされていない、または無効な場合は処理をスキップ（安全装置）
    if (!model || !model->cpuModel) {
        return; 
    }
    
    // テクスチャのSRVハンドル（GPU用）を解決
    auto srvHandle = engine_->GetTextureManager()->Resolve(myTextureHandle_);
    
    // 実際の描画処理へ...
    model->Draw(srvHandle);
}
```

## GPU カリング (GPU Culling & ExecuteIndirect) の利用方法

本エンジンでは、大量の同一モデル（がれき、草、パーティクルなど）を描画する際のCPU負荷（フラスタムカリングやメモリ転送）を劇的に削減するため、**Compute Shader による GPU フラスタムカリング** をサポートしました。

この機能を有効にすると、CPU側でのカリング判定がスキップされ、GPUが自身で「カメラに映っているオブジェクト」だけを判定し、`ExecuteIndirect` を通して一括描画するようになります。

### 利用方法 (コンポーネントからの利用)
最も簡単な方法は、`ModelBatchRendererComponent` （またはそれを内部で生成するマネージャークラス）に対して、初期化時にフラグを有効化することです。

```cpp
#include "Framework/Component/Renderer/ModelBatchRendererComponent.h"

// 1. バッチレンダラーコンポーネントの取得または追加
auto batchRenderer = gameObject->AddComponent<ModelBatchRendererComponent>();

// 2. GPUカリングを有効にする
batchRenderer->SetUseGPUCulling(true);
```

### 利用方法 (プログラム/バッチからの直接利用)
`ModelBatch` などのバッチクラスを直接生成して描画している場合も、同様に `SetUseGPUCulling(true)` を呼び出すだけです。

```cpp
#include "Renderer/Object/Batch/ModelBatch.h"

// 1. バッチの生成
ModelBatch myBatch;
myBatch.Initialize("DebrisModel");

// 2. GPUカリングを有効化
myBatch.SetUseGPUCulling(true);

// 3. インスタンスの追加 (CPU側でTransformを設定)
for (int i = 0; i < 10000; ++i) {
    myBatch.AddInstanceWorld(Matrix4x4::MakeTranslation(Vector3(i * 1.0f, 0, 0)));
}

// 4. 描画
// 内部で自動的にGPUカリング用Compute Shaderが実行され、その後ExecuteIndirectで描画されます
myBatch.Draw();
```

### 注意点・制限事項
- GPUカリングは、**1000個以上の大量のインスタンス**を描画する場合に効果を発揮します。少数のオブジェクトに対しては、逆にCompute Shaderのディスパッチオーバーヘッドが上回る可能性があります。
- 描画対象のオブジェクトには、ローカル空間での正確な **バウンディングスフィア (BoundingSphere)** が設定されている必要があります（`GetBoundingSphereRadius` の値がカリングに使用されます）。

---

## GPU Skinning によるアニメーション最適化

本エンジンでは、スケルタルアニメーション（ボーン変形）の処理を、従来のCPU計算や頂点シェーダ(VS)で行うのではなく、事前に **Compute Shader (`Skinning.CS.hlsl`) で並列計算（プレコンピュート）** する最新のアーキテクチャ（GPU Skinning）を採用しています。

### アーキテクチャのメリット
- **CPU負荷の完全開放**: CPU側ではアニメーションの再生時間と「行列パレット（Matrix Palette）」をGPUへ転送するだけで済み、数千〜数万の頂点に対する行列乗算からCPUが完全に解放されます。
- **描画パイプラインとの親和性**: Compute Shaderで変形した後の頂点データがバッファに書き出されるため、その後のGPUフラスタムカリングなどにそのまま使い回すことができる先進的な設計です。

開発チームの皆様は、通常通りモデルを読み込んで再生するだけで、裏側で自動的にこの恩恵を受けることができます。

---

## スキニングメッシュのマルチマテリアル個別上書き機能 (Material Override)

`SkinnedMeshRendererComponent` は、モデルが持つ複数のマテリアルスロット（部位）に対して、個別にパラメータを上書き（Override）する機能を持っています。
Unreal Engine や Unity の「マテリアルインスタンス」に近い概念であり、特定のスロットだけ色を変えたり、質感を調整したりすることが可能です。

### エディタ (Inspector) での編集方法

1. 対象の `GameObject` を選択し、Inspector の `SkinnedMeshRendererComponent` パネルを開きます。
2. **`Materials (Slots)`** というヘッダーをクリックして展開します。
3. モデルが持つスロットの数だけ `Element 0`, `Element 1` ... と表示されます。
4. 色や質感を変更したい Element を開き、**`Override Material`** にチェックを入れます。
5. 出現する `Color`, `Roughness`, `Metallic`, `Enable Lighting` 等のプロパティを編集します。
6. 設定内容はシーン（`Ctrl+S`）に保存・復元されます。

### C++ ゲームロジックからの動的変更

ゲーム中に特定のアクターのパーツ色（例：ダメージを受けたときに赤くする等）を変更したい場合、スクリプトから以下のようにアクセスできます。

```cpp
auto skinnedMesh = gameObject_->GetComponent<SkinnedMeshRendererComponent>();
if (skinnedMesh) {
    ObjMaterial overrideMat;
    overrideMat.color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤色
    overrideMat.roughness = 0.2f;
    overrideMat.metallic = 1.0f;
    
    // スロット0（Element 0）に対してマテリアルを上書き
    skinnedMesh->SetMaterialOverride(0, overrideMat);
}
```

上書きを解除し、モデル標準の共有マテリアルに戻したい場合は、`RemoveMaterialOverride(slotIndex)` を呼び出します。

```cpp
skinnedMesh->RemoveMaterialOverride(0);
```

### パフォーマンスに関する注意点
この機能は GPU 定数バッファ（Constant Buffer）を用いてスロットごとのマテリアルパラメータを個別に転送・描画する設計になっています（Drawコールはスロットごとに発生します）。
AAA規模の大規模なインスタンシングが必要な場合は、現状の Drawコール単位の処理から Material ID マップ等を利用した最適化へ拡張できる設計基盤となっています。

---

## 大量オブジェクトの最適化 (VirtualEntityManagerComponent) の利用方法

本エンジンでは、数万個レベルの大量のオブジェクト（がれき、草、弾幕など）を最適化して描画・管理するためのシステムとして、**`VirtualEntityManagerComponent`** (Instance Replacement / Promotion パターン) をサポートしました。

このコンポーネントを使用すると、普段は `GameObject` を実体化せずに軽量な「行列データ（Virtual Transform）」としてのみ管理・GPU描画し、プレイヤーが干渉した瞬間など **本当に必要なときだけ本物のアクター（GameObject）に昇格（Promote）させる** ことができます。

### 導入手順
1. **マネージャーの準備**:
   ゲーム側の管理クラス（例: `DebrisManagerComponent`）で、`VirtualEntityManagerComponent` をアタッチして初期化します。

```cpp
#include "Framework/Component/VirtualEntity/VirtualEntityManagerComponent.h"

// 1. コンポーネントの追加
auto virtualManager = gameObject_->AddComponent<VirtualEntityManagerComponent>();

// 2. プールサイズと生成ファクトリの登録
auto factory = [this]() -> std::shared_ptr<GameObject> {
    auto obj = std::make_shared<GameObject>("MyEntity");
    obj->AddComponent<DebrisComponent>(); // 物理挙動などをアタッチ
    obj->SetIsActive(false);
    gameObject_->AddChild(obj);
    return obj;
};
virtualManager->Setup(500, factory); // 最大500個までは同時に実体化可能
```

2. **データの追加（実体化しない）**:
```cpp
// 座標や回転だけを登録し、仮想インスタンスIDを受け取る
int id = virtualManager->AddVirtualInstance(Vector3(10, 0, 0));
```

3. **データの更新 (Data-Oriented Update)**:
   アニメーションなどを適用したい場合は、`GetVirtualInstances()` で配列を取得し、毎フレーム直接書き換えます。実体がないため超高速に処理されます。
```cpp
auto& instances = virtualManager->GetVirtualInstances();
for (auto& vi : instances) {
    if (!vi.isPromoted_ && !vi.isDestroyed_) {
        // 例：Y座標をフワフワさせる
        vi.position_.y += std::sin(time) * 0.1f;
    }
}
```

4. **昇格 (Promote) と 降格 (Demote)**:
   プレイヤーが近づいた、攻撃を当てた等のタイミングで、IDを指定して本物の `GameObject` に昇格させます。
```cpp
// id を指定してGameObjectをプールから取得し、仮想インスタンスの座標を同期する
auto realObj = virtualManager->Promote(id);
if (realObj) {
    // 物理演算を有効化したり、ターゲットを追従させたりする
}

// 用済みになったらデータに戻す
virtualManager->Demote(id);
```

このシステムと前述の「GPU Culling」を組み合わせることで、数万個のオブジェクトがあっても 60FPS を余裕で維持できるパフォーマンスを実現できます。

---

## TransformComponent の使い方 (カプセル化と遅延評価)

本エンジンでは、描画・物理・ゲームロジック間のキャッシュ効率および同期安全性を高めるため、**`TransformComponent` のアーキテクチャがカプセル化（Data-Oriented Design対応）されました。**

従来のように `transform->position_` のようなパブリックメンバへの直接代入・参照は禁止されており、代わりに **Getter / Setter** を使用する必要があります。

### 主なAPI
```cpp
auto transform = gameObject_->GetComponent<TransformComponent>();

// --- 取得 (Getter) ---
// ローカル座標系
Vector3 localPos = transform->GetPosition();
Vector3 localRot = transform->GetRotation(); // オイラー角(ラジアン)
Vector3 localScl = transform->GetScale();

// ワールド座標系（親のTransformを加味した最終結果）
Vector3 worldPos = transform->GetWorldPosition();
Vector3 worldRot = transform->GetWorldRotation();
Vector3 worldScl = transform->GetWorldScale();

// ワールド空間の方向ベクトル（正規化済み）
Vector3 right   = transform->GetWorldRight();
Vector3 up      = transform->GetWorldUp();
Vector3 forward = transform->GetWorldForward();

// --- 更新 (Setter) ---
// 値を更新すると、内部で Dirty フラグ (isLocalDirty_) が立ちます
transform->SetPosition(Vector3(10, 5, 0));
transform->SetRotation(Vector3(0, Math::ToRadian(90), 0));
transform->SetScale(Vector3(2, 2, 2));
```

### 【重要】Dirty フラグと遅延評価 (Lazy Evaluation) の仕組み
Setter を通じて座標を変更しても、**その瞬間にすべての行列計算が行われるわけではありません。**
内部では「Dirty（変更あり）」というフラグだけが立ち、実際に `GetWorldMatrix()` や描画処理から行列が要求されたタイミングで、**1回だけ（キャッシュとして）再計算** される遅延評価の仕組みが導入されています。

これにより、同じフレーム内で何度座標を変更しても、無駄な行列計算（sin/cosや行列乗算）が走らないため、非常に高速に動作します。
※ 特別な理由がない限り、自分で `ComputeMatrix()` を呼び出す必要はありません。エンジン側の `BaseScene::Update()` の直後に一括で最新化されます。

---

## 汎用2Dプリミティブ描画 (Primitive2DObject / Primitive2DRendererComponent) の利用方法

2D空間での汎用的な図形（四角形、円、線、リングなど）を簡単に描画するための `Primitive2DObject` および、それをエディタで直感的に扱える `Primitive2DRendererComponent` が追加されました。
デバッグ表示、UIの枠線、シンプルな2Dエフェクトなどに最適です。

### コンポーネントからの利用
GameObjectに `Primitive2DRendererComponent` をアタッチすることで、Inspectorからリアルタイムにパラメータを変更できます。

- **Shape Type**: `Rect` (四角), `Triangle` (三角), `Circle` (円), `Ring` (ドーナツ状), `Line` (線) から選択可能。
- **Size**: ベースとなる描画サイズ（TransformのScaleと掛け合わされて最終的なサイズになります）。
- **Pivot**: 描画の基準点 (0.0 ～ 1.0)。デフォルトは 0.5, 0.5（中心）。
- **Subdivision**: `Circle` や `Ring` などの滑らかさ（頂点分割数）を設定。
- **Thickness**: `Ring` や `Line` の線の太さを設定。
- **Texture / Color**: ドラッグ＆ドロップで任意のテクスチャを貼り付けたり、全体の色や透明度を変更可能。
- **TopMost**: 他のすべての描画物よりも最前面に描画するかどうか。

### プログラムからの直接利用
コンポーネントを使わず、直接 `Primitive2DObject` を生成して描画することも可能です（これまでの `Circle2D` の完全な上位互換として動作します）。

```cpp
#include "Renderer/Object/2D/Primitive/Primitive2DObject.h"

// 1. 生成と初期化
Primitive2DObject myShape;
myShape.Initialize(Primitive2DType::Circle); // 初期形状を円に指定

// 2. パラメータの変更
myShape.SetSize(Vector2(200.0f, 200.0f));
myShape.SetColor(Vector4(1.0f, 0.0f, 0.0f, 0.5f)); // 半透明の赤
myShape.SetThickness(5.0f); // 線の太さ（Line, Ring の場合）
myShape.SetPosition(640.0f, 360.0f, 0.0f); // 画面中央に配置

// 3. 毎フレーム Update と Draw を呼ぶ
myShape.Update();
myShape.SyncBeforeDraw(); // GPUへのデータ転送を確実に行う
myShape.Draw();
```

※ `Primitive2DObject` は内部で頂点バッファを動的に再構築するため、パラメータ（種類や分割数など）を変更した場合は、次のフレームで自動的にGPUへの再アップロードが行われます。

---

## 半透明・エフェクトオブジェクトの描画とZソート

本エンジンでは、地形やキャラクターなどの不透明オブジェクトの後に、オーラやレーザーなどの半透明エフェクトを正しい順番（奥から手前）で描画するための **独立した半透明描画パス (MainTransparentPass)** をサポートしました。

これまで半透明オブジェクトを描画する際、Zバッファへの書き込み（DepthWrite）をDisableにすると、後から描画される不透明オブジェクトに上書きされて見えなくなる問題がありましたが、この機能を利用することで正しく描画されます。

### 利用方法

半透明や加算合成で描画したい Primitive3DObject （またはそれを保持するRendererComponent）に対して、初期化時に SetIsTransparent(true) を設定し、同時にPSO設定で DepthWrite::Disable を指定します。

```cpp
#include "Renderer/Object/3D/Primitive/Primitive3DObject.h"

// 1. オブジェクトの初期化
auto aura = std::make_shared<Primitive3DObject>();
aura->Initialize(PrimitiveType::Sphere);

// 2. カスタムPSOの適用 (DepthWrite を Disable にする)
auto pso = engine->GetPSOManager()->GetPSO("EnergyCore", BlendMode::kBlendModePremultiplied, PSOManager::DepthWrite::Disable, PSOManager::CullMode::Back);
aura->SetCustomPSO(pso);

// 3. 半透明フラグを有効にする (重要！)
// これにより、不透明オブジェクトをすべて描き終わった後に、カメラからの距離でソートされて描画されます。
aura->SetIsTransparent(true);
```

### 注意点
- SetIsTransparent(true) を設定したオブジェクトは、自動的にカメラからの距離（distanceToCamera）を計算し、**Z値の降順（Back-to-Front）**でソートされて描画されます。
- 不透明な通常のモデルに SetIsTransparent(true) を設定しないでください（Early-Zカリングなどの恩恵が受けられず、パフォーマンスが低下します）。

---

## デバッグ描画と当たり判定 (CollisionManager)

コライダー（AABB, Sphere, OBB）のデバッグ描画（ワイヤーフレーム表示）は、各コンポーネント内で個別に実装・描画する必要はありません。
すべてのコライダーのデバッグ描画は、`CollisionManager::DrawDebug()` にて一元管理・一括描画されるアーキテクチャに変更されています。

- **デバッグ表示の自動化**:
  `ColliderComponent` を継承してコンポーネントを作成し、`GetWorldAABB()` などの形状取得メソッドを正しくオーバーライドすれば、エディタ上で自動的にワイヤーフレームが表示されます。
- **自分で `DrawDebug` を呼ばない**:
  各コンポーネント内に独自の `DrawDebug` 等の描画命令（PrimitiveManager等の呼び出し）を記述すると、描画が重複したり描画ステートが壊れる原因となるため、当たり判定の描画は完全にマネージャに委譲してください。

### センサーとしての利用 (RaycastComponent)
`CollisionManager::RaycastAsync` などの非同期レイキャストの他に、`GameObject` の向いている方向（ローカルのZ軸前方など）に毎フレーム自動的にレイキャストを行い、障害物やターゲットを検知し続けるセンサーとして `RaycastComponent` が用意されています。

```cpp
auto raycast = gameObject_->AddComponent<RaycastComponent>();
// エディタ上で maxDistance (最大距離) や mask (対象レイヤー) を設定可能
```
このコンポーネントを使用すると、敵の視界判定や、銃口からの即着弾判定などをインスペクター上で視覚的に調整しながら実装できます。

---

## 3Dアニメーションと Root Motion の利用方法

本エンジンでは、3Dキャラクターなどのスケルタルアニメーション（ボーン変形）を描画・制御するためのコンポーネントシステムとして、`SkinnedMeshRendererComponent` と `AnimatorComponent` の分離アーキテクチャを採用しています。
これにより、アニメーションのロジック（再生やブレンド）と描画の責務が完全に分離され、高速な処理が可能になっています。

### 基本的なアニメーション再生

アニメーション付きのモデル（GLTF / FBX）をシーンに配置し再生する場合は、GameObjectに2つのコンポーネントをアタッチします。

```cpp
// 1. 描画コンポーネントの追加とモデルのロード
auto renderer = gameObject_->AddComponent<SkinnedMeshRendererComponent>();
renderer->LoadModel("sample/walk.gltf");

// 2. アニメーション制御コンポーネントの追加
auto animator = gameObject_->AddComponent<AnimatorComponent>();

// 3. アニメーションの再生 (ファイル名, ループフラグ)
animator->Play("sample/walk.gltf", true);
```

### アニメーションのクロスフェード (Blend Tree)

走っている状態から歩く状態へ切り替わる際など、モーションがパキッと切り替わるのを防ぐため、本エンジンは**球面線形補間（Slerp）を用いた自動クロスフェード機能**を備えています。

```cpp
auto animator = gameObject_->GetComponent<AnimatorComponent>();

// 第3引数にフェード時間（秒）を指定することで、現在のポーズから次のポーズへ滑らかに遷移します。
// 例：1.0秒かけて "walk" から "sneakWalk" に滑らかに移行する
animator->Play("sample/sneakWalk.gltf", true, 1.0f);
```

### 足滑りを防ぐ Root Motion (ルートモーション)

従来のアニメーション再生では「その場で足踏みするアニメーション」を再生しながら、プログラム側でキャラクターの座標を移動（Translate）させる必要がありましたが、これでは足の動きと実際の移動速度が合わず「足が滑っている（ムーンウォーク）」ように見えてしまう問題がありました。

本エンジンは、アニメーションデータ自体が持っている「移動量（ルートボーンの差分）」を自動抽出し、それを直接 GameObject の `TransformComponent` の座標に還元する **Root Motion** 機能をサポートしています。

```cpp
auto animator = gameObject_->GetComponent<AnimatorComponent>();

// Root Motion を有効化（インスペクタのUIからもチェックボックスで切り替え可能）
animator->SetApplyRootMotion(true);

// これ以降、"walk.gltf" などを再生すると、アニメーションの移動量に連動して
// 自動的に GameObject そのもの（Transform）が移動するようになります。
// （※開発者が手動で Transform::SetPosition を呼んでキャラクターを前進させる必要はありません）
```

これにより、モーションデザイナーが意図した通りの「絶対に足が滑らない、物理的に正確な移動」が実現されます。

---

## 統合テスト環境 (DebugScene) の利用ルール

本エンジンには、エンジンコアの機能テストや描画テストを行うための独立したサンドボックス環境として **`DebugScene`** が `IrufemiEngine/Framework/` 配下に統合されています。（以前は各ゲームアプリケーション側に重複して存在していましたが、リファクタリングによりエンジン側に一元化されました。）

### DebugScene の目的と立ち位置
エンジンの機能追加（新しいコンポーネント、シェーダー、アニメーション機構など）を行う際は、**いきなりゲーム本編のシーン（`InGameScene` など）に組み込むのではなく、まずはこの `DebugScene` にテスト用のオブジェクトを配置して単体テスト・動作確認を行う** ことが強く推奨されます。

これにより、ゲーム特有の複雑なロジック（ステートマシンやカメラ制御など）の干渉を受けずに純粋なエンジンのバグ切り分けが可能になります。

### テスト用オブジェクトの追加と管理 (Activationパネル)
`DebugScene` に新しいテスト要素を追加する際は、常に描画し続けるのではなく、ImGuiのトグルスイッチを使って「必要なときだけ表示・更新」できるように実装してください。

1. **フラグとオブジェクト変数の定義 (`DebugScene.h`)**:
   ```cpp
   std::unique_ptr<GameObject> myTestObj_ = nullptr;
   bool isActiveMyTest_ = false;
   ```

2. **Activation ウィンドウへの登録 (`DebugScene.cpp` の Update 内)**:
   ```cpp
   if (ImGui::Begin("Activation")) {
       ImGui::Checkbox("My Test Feature", &isActiveMyTest_);
   }
   ImGui::End();
   ```

3. **遅延生成と更新・描画ロジック**:
   チェックボックスがONにされた瞬間（または初回フレーム）に初めてオブジェクトを生成（Instantiate/make_unique）し、チェックが入っている間だけ Update / Draw を呼ぶようにします。
   ```cpp
   if (isActiveMyTest_) {
       if (!myTestObj_) {
           myTestObj_ = std::make_unique<GameObject>("TestObj");
           myTestObj_->AddComponent<MyNewComponent>();
       }
       myTestObj_->Update();
   }
   ```

※なお、`DebugScene` はあくまでエンジンのテスト機能であるため、特定のゲーム（シューティングやアクション等）のプレイヤーキャラや敵キャラの仕様をそのまま持ち込むことは禁止されています。（チーム開発ルール 1. アーキテクチャと関心の分離 に従うこと）

---

## Bindless Resources (Descriptor Indexing) 完全移行について

現在、IrufemiEngine はパフォーマンス向上を目的とした **Bindless Resources** (Descriptor Indexing) への移行を完了しました。

### テクスチャバインドのルール（完全移行後）
C++側の基盤構築およびHLSL側の対応がすべて完了しており、全テクスチャが巨大な配列 (gTextures および gTextureCubes) に格納されています。
**HLSL（シェーダー）側はレガシーな register(t0) への個別バインドを廃止し、定数バッファ（MaterialやParams）経由で渡された textureIndex を用いて配列からテクスチャを参照します。**

エンジンのルートシグネチャからは互換性維持のためのレガシースロット (RootSlot::Texture, RootSlot::EnvMap) が完全に削除されました。
これにより、各バッチ処理やレンダラーの実装から手動でのテクスチャバインド処理（SetGraphicsRootDescriptorTable）は不要になっています。

- **C++側の変更**: 各種描画コマンド（PrimitiveBatch, ModelBatch, GPUParticleSystem等）や RenderPackets から textureHandle が削除されました。テクスチャの切り替えは自動的に Material バッファ内の textureIndex の更新によって行われます。
- **PostProcess側の変更**: PostProcessManager は各エフェクトの描画前に PostProcessBindlessParams 定数バッファ (b1) を更新し、mainTextureIndex および extraTextureIndex をシェーダーへ渡します。シェーダー内では gTexture マクロが自動的にインデックス解決を行うため、既存のエフェクト計算コードを書き直すことなくBindlessの恩恵を受けられます。

---

## 【開発コラム】BVHとデータ指向設計における「最適なオブジェクトプール」とは？

ゲーム開発において、大量のガレキや敵を管理する際に `ObjectPool<T>` (ポインタの使い回し) を使うのは常識ですが、**BVH（Bounding Volume Hierarchy）のような極めて高速なツリー走査が求められるシステムにおいては、ポインタベースのプールは逆効果（フラグメンテーションによるキャッシュミス）となります。**

そのため、本エンジンの `DynamicBVH` 等では、ポインタの代わりに **「インデックスベースの配列プール (Array-based Free List Pool)」** を採用しています。

1. **完全な連続メモリ (`std::vector<BVHNode>`)**:
   最初に `reserve(20000)` 等で巨大な連続メモリを確保し、`push_back` していきます。メモリが完全に連続しているため、ツリー走査時の L1/L2 キャッシュヒット率が極大化します。
2. **ポインタ(8バイト)からインデックス(4バイト)への圧縮**:
   `BVHNode* leftChild;` の代わりに `int32_t leftChildIndex;` を使うことで、ノードサイズを半減させ、一度にキャッシュに乗るデータ量を倍増させています。
3. **Free List による再利用 (プーリング)**:
   ノードが削除された場合は、そのインデックスを空き番号リスト (`freeNodes`) に入れ、次の挿入時にそこを上書き再利用します。

つまり、**名前が `std::vector` なだけで、内部的にはメモリアロケーションを一切発生させない「BVHに特化した究極のオブジェクトプール」として機能しています。** このように、用途に応じて「ポインタベース」と「インデックスベース」のプールを使い分ける設計が、AAAエンジンのパフォーマンスを支えています。
---

## 【トラブルシューティング】過去の深刻なバグと対応履歴

## カメラ・ユーティリティ・ロジックコンポーネント

本エンジンには、開発を効率化する以下の強力なコンポーネントが標準で用意されています。

### 1. CameraComponent (基本カメラ)
3Dシーンを描画するための基本となるカメラコンポーネントです。ビュー行列やプロジェクション行列（FOV・Near/Farクリップ）を管理します。シーンには最低1つのカメラが必要です。

### 2. TargetFollowComponent (カメラ追従)
指定した `GameObject` を一定距離と角度で追従するカメラ用コンポーネントです。（通常は `CameraComponent` と併用します）
- **追従遅延 (Delay)**: 即座に追従するだけでなく、滑らかに遅れて追従するシネマティックなカメラワークをサポートしています。

### 3. SplineComponent (スプライン軌道)
複数のウェイポイントを Catmull-Rom スプラインで滑らかに結び、任意の進行度(t)での座標や接線（進行方向）を取得できる汎用コンポーネントです。
- 敵のレール移動、カットシーンのカメラワーク、曲がりくねったレーザーの描画等に有用です。
（※ `SplineNodeComponent` を子オブジェクトとして配置することでエディタ上で軌道を編集できます）

### 4. BoneAttachmentComponent (骨格追従)
スキニングアニメーションモデルが持つ特定のボーン（`targetBoneName`）に、別のオブジェクトを追従させる機能です。
- キャラクターに武器を持たせたり、エフェクトを特定の部位（手や剣先）に追従させる際に必須となります。

---

## UIシステム (Canvas & 2D描画)

ゲームのHUDやメニュー画面を構築するために、階層的なUIシステムが用意されています。

### 1. CanvasComponent (UIルート)
すべてのUI要素の親となるコンポーネントです。画面解像度の変更に伴う自動スケーリングや、アスペクト比の維持を担当します。UIを作成する際は、必ずルートの `GameObject` にこのコンポーネントをアタッチしてください。

### 2. SpriteRendererComponent (2D画像描画)
UIとして2Dテクスチャ（スプライト）を描画します。色や透明度、アンカーポイント（Pivot）の変更が可能です。

### 3. TextRendererComponent (テキスト描画)
TrueTypeフォント（`.ttf`）を用いて、画面上に文字列を描画します。サイズや色、配置揃え（左寄せ・中央揃えなど）を調整できます。

### 4. ButtonComponent (インタラクション)
ボタンとしてのクリック判定と、ホバー時・クリック時のコールバック処理を管理します。
```cpp
auto button = uiObject->AddComponent<ButtonComponent>();
button->SetOnClickCallback([]() {
    // ボタンがクリックされたときの処理
    Log::OutPutLog(std::cout, "Button Clicked!\n");
});
```

---

## ScreenCaptureManager (スクリーンショット・メタデータ)

画面のキャプチャを安全に行うためのシステムです。UIを含めない純粋なシーンのみ (`PreUI`) や、UIを含めた最終画面 (`PostUI`) の出力、さらにはアルファチャンネルや深度バッファのみの出力に対応しています。
- **メタデータ連携**: キャプチャ時にエンジン内の状態をJSONメタデータとして同時に出力する機能（`RecordMetadata`）も備わっており、機械学習用データセットの作成などに応用可能です。

---

### 1. 独自キャッシュからの復元（SSOバッファ破壊）と非同期ロードのすり抜けによるアクセス違反

- **現象**: GLTF などのアニメーションモデルを非同期ロードする際、`std::_Tree::empty()` 等の STL 内部（`m->cpuModel->skinClusterData.empty()` など）で `0xC0000005` (アクセス違反) や `0xB8` 等の不正なポインタ参照が発生し、高い頻度でクラッシュする。
- **原因**: 以下の2つの致命的なバグが連鎖して起きていた。
  1. **SSO（Small String Optimization）バッファの破壊**: `ModelSerializer` でバイナリキャッシュ (`.ibin`) から `std::string` を復元する際、`ifs.read(str.data(), size)` で直接書き込んでいた。古いキャッシュや中途半端なデータが読み込まれた際にSSOの管理領域やヒープが破壊され、直後の `skinClusterData`（std::map）のメモリ構造が壊れていた。
  2. **非同期ロードのすり抜け**: 非同期でモデルをロードしている最中に、`AnimatedMeshObject::Update` や `Draw` 側で `m->cpuModel` が完全に構築される前（`nullptr` の状態）にアクセスする防御抜けが存在した。
- **当初の誤認**: エラーが `nlohmann::json` のパース処理周辺で発生しているように見えたため、Assimp 内部の `nlohmann::json` とエンジン側の `nlohmann::json` の ODR（One Definition Rule）違反が疑われ、ABIタグを変更する対策が取られたが、検証の結果これは濡れ衣（無関係）であることが判明した。
- **解決策**:
  - `ModelSerializer` における文字列の復元を `std::vector<char>` 経由で行うように修正し、すべての読み込み関数に `ifs.fail()` の厳格なエラーチェックを追加した。
  - キャッシュフォーマットのバージョン (`kVersion`) をインクリメントし、古いキャッシュを無効化した。
  - `AnimatedMeshObject` の `Update` および `Draw` メソッドの冒頭に、`m->status.load() != Loaded` および `!m->cpuModel` の場合は即座にリターンする「強力なフェイルセーフ（早期リターン）」を追加した。さらにモデル切り替え時に `meshResources_.clear()` を行うようにした。
- **教訓**: バイナリシリアライゼーションを行う際は、STL コンテナの内部実装（SSOなど）に依存する直接的なメモリ書き込みを避け、安全なバッファを経由すること。また、非同期ロード時のステート管理（排他制御や状態チェック）は、描画ループ側でも徹底すること。
