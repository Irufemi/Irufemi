# IrufemiEngine 取扱説明書 (Manual)

このドキュメントは、IrufemiEngineを利用してゲームを開発するチームメンバーのための総合マニュアルです。
各機能の役割と、すぐに使えるコードスニペット（コピペ用コード）をまとめています。

---

## 0. プロジェクト構造とコーディングルール (Project Structure)

新しくコードを書いたり、リソースを追加する際は以下の配置ルールに必ず従ってください。

- **`IrufemiEngine/` (エンジンコア)**
  - 描画パイプラインや汎用的なマネージャーが置かれます。**ゲーム特有のロジックやアクターは絶対にここに書かないでください。**
- **`Application/` (ゲームロジック)**
  - プレイヤーの動き、敵のAI、各種シーン（Title, InGame等）の処理はすべてここに作成します。
- **`resources/` (リソースデータ)**
  - 3Dモデル（`.obj`, `.gltf`）やテクスチャ（`.png`）、音声（`.wav`）は必ずこのフォルダ以下の適切なディレクトリ（`model/`, `ui/`, `audio/` 等）に配置してください。

---

## 1. エンジンの基本アーキテクチャ (Core Architecture)

### 1.1 IrufemiEngine クラス
エンジン全体を統括するコアクラスです。`WinApp`（ウィンドウ管理）や `DirectXCommon`（DirectX12初期化）を保持し、メインループ（`Update` と `Draw`）を回します。ゲームアプリケーション全体で1つのインスタンスのみが存在します。

### 1.2 SceneManager と IScene (シーン管理)
ゲームの画面（タイトル、インゲーム、リザルトなど）を切り替えるための仕組みです。
新しいシーンを追加する場合は `IScene` または `BaseScene` を継承したクラスを作成します。

#### シーンの作り方
```cpp
#pragma once
#include "Framework/BaseScene.h"
#include <memory>

class ExampleScene : public BaseScene {
public:
    ~ExampleScene() override = default;
    
    // シーン遷移時に一度だけ呼ばれる（リソース読み込みなど）
    void Initialize(IrufemiEngine* engine) override;
    
    // 毎フレーム呼ばれる（ロジックの更新）
    void Update() override;
    
    // 毎フレーム呼ばれる（描画リクエストの送信）
    void Draw() override;

    // --- ライフサイクル関数（必要に応じてオーバーライド） ---
    // void OnEnter() override;   // アクティブになった時
    // void OnExit() override;    // 非アクティブ・破棄される直前
    // void OnSuspend() override; // 上に別のシーンがPushされた時
    // void OnResume() override;  // 上のシーンがPopされ最前面に戻った時

private:
    // コンポーネント指向を利用するため、特定のオブジェクトは SceneManager (GameObject) 経由で管理します
};
```

#### シーン遷移（自動データ駆動遷移と手動遷移）
シーンを切り替えるには主に2つの方法があります。

**1. データ駆動の自動遷移 (推奨)**
あらかじめエディタで作った `Title.json` などを指定して画面を遷移します。C++で専用のシーンクラスを作る必要はありません。
```cpp
// "Title" (Title.json) を読み込んで遷移。トランジション効果は Fade で 1.0秒かける
engine_->GetSceneManager()->LoadScene("Title", SceneTransition::Type::Fade, 1.0f);
```

**2. C++クラスによる手動遷移**
`BaseScene` を継承した専用クラスへ遷移します。
```cpp
if (isClear) {
    // 登録済みの "ClearScene" クラスに遷移する
    engine_->GetSceneManager()->TransitionTo("ClearScene", SceneTransition::Type::Fade, 1.0f);
}

// 現在のシーンの上に一時的なシーン（ポーズ画面など）を重ねる場合
// engine_->GetSceneManager()->PushScene("PauseScene");

// 重ねたシーンを終了し、元のシーンに戻る場合（PauseScene側で呼ぶ）
// engine_->GetSceneManager()->PopScene();
```

#### シーンのロード状態と一つ前のシーンの取得
ロード画面の描画や、ポーズ画面から元のシーンへ戻る際の判定に便利なAPIが用意されています。
```cpp
// チラつき防止機能付きで、現在ローディング画面を描画すべきか判定
if (engine_->GetSceneManager()->ShouldDrawLoadingScreen()) {
    // ローディング画面の描画処理
}

// PauseScene などの上に重なるシーンから、呼び出し元のシーン名を取得する
std::string prevScene = engine_->GetSceneManager()->GetPreviousSceneName();
if (prevScene == "GameScene") {
    // GameScene から呼ばれた場合の特別な処理
}
```

#### GameObject の検索（名前、タグ、ID）
シーン内に存在する GameObject を取得したい場合、用途に合わせて以下の3つの検索機能を利用できます。

**1. 名前で検索 (`FindGameObject`)**
特定の名前を持つオブジェクトを1つ探す場合に使用します。
```cpp
auto playerObj = scene->FindGameObject("Player");
```
※同名のオブジェクトが複数存在する場合は、最初に見つかったものが返されます。

**2. タグで一括検索 (`FindGameObjectsWithTag`)**
「Enemy」や「Obstacle」など、特定の役割を持つオブジェクトをまとめて取得したい場合に使用します。タグは `GameObject::SetTag("Enemy")` やエディタから設定できます。
```cpp
// "Enemy" タグを持つすべてのオブジェクトを取得
std::vector<std::shared_ptr<GameObject>> enemies = scene->FindGameObjectsWithTag("Enemy");
for (auto& enemy : enemies) {
    // 敵全体に対する処理
}
```

**3. インスタンスIDで検索 (`FindGameObjectByID`)**
オブジェクトの生成時に自動で割り当てられる一意のID（`uint64_t`）を利用して、確実に特定のインスタンスを取得します。
```cpp
uint64_t targetId = targetObj->GetInstanceID();
// ... 後でIDを使って再取得する
auto obj = scene->FindGameObjectByID(targetId);
```

#### 動的生成 (Clone) 時の自動命名ルール
`GameObject::Clone()` を使用してオブジェクトを複製すると、エディタ上の識別や名前検索の競合を防ぐため、名前に自動でサフィックス（`(1)` など）が付きます。
* 例: `"Enemy"` を Clone → `"Enemy(1)"`
* さらに Clone → `"Enemy(2)"`
これにより、プレハブから動的生成した敵などもインスペクター上で個別に識別しやすくなっています。

### 1.3 RenderGraph と DrawManager (描画パイプライン)
本エンジンの描画は **RenderGraph（レンダーグラフ）** という仕組みで自動管理されています。
ユーザーが `Draw()` を呼ぶと、すぐに画面に描画されるわけではなく、**DrawManagerのキュー（予約リスト）に登録（Submit）** されます。その後、エンジン側が適切な順序（Opaque → Transparent → UIなど）でまとめてGPUへ描画命令を出します。

### 1.4 カメラの操作 (CameraManager / Camera)
3D空間を描画するための「視点」を管理します。

```cpp
auto* cameraManager = engine_->GetCameraManager();
auto* camera = cameraManager->GetActiveCamera();

// カメラの位置と注視点を設定
camera->SetTranslate({ 0.0f, 5.0f, -10.0f });
camera->SetTarget({ 0.0f, 0.0f, 0.0f });

// 行列を更新（位置を変更したら必ず呼ぶ）
camera->UpdateMatrix();

// ※デバッグカメラへの切り替えについて
// 以前は CameraManager で切り替えていましたが、現在は BaseScene に統合されています。
// ImGuiのデバッグタブ「Camera & Lights」から "Debug Camera Mode" のチェックを入れるか、
// コード内で `isDebugCameraMode_ = true;` とすることでデバッグカメラが有効になります。
```

---

## 2. 描画・オブジェクトシステム (Rendering System)

画面にモノを表示するための主要なクラス群です。

### 2.1 描画コンポーネント (Renderer Components)
画面にオブジェクトを表示するためには、`GameObject` に適切なレンダラーコンポーネントをアタッチします。

#### MeshRendererComponent (3Dモデル)
`.obj` や静的な `.gltf` 形式の3Dモデルを描画します。
*(※ボーンアニメーションを持つモデルを描画する場合は、現状はコンポーネントではなく手動生成の `AnimationModel` クラスを使用してください。コンポーネント版は将来追加予定です。)*
```cpp
auto obj = std::make_shared<GameObject>("Enemy");
auto* renderer = obj->AddComponent<MeshRendererComponent>();
renderer->LoadModel("enemy/enemy.obj"); // resources/model/ 以下のパス
// scene->AddGameObject(obj);
```

#### ModelBatchRendererComponent (大量の同一モデル)
草や破片など、同じモデルを大量に描画する際に使用します。インスタンシング描画によりGPU負荷を激減させます。
```cpp
auto obj = std::make_shared<GameObject>("GrassBatch");
auto* batchRenderer = obj->AddComponent<ModelBatchRendererComponent>();
batchRenderer->LoadModel("env/grass.obj"); // resources/model/ 以下のパス
// 描画するインスタンスの追加・更新処理等は Component 内で行います
```

#### PrimitiveRendererComponent (基本図形)
モデルデータなしでキューブ、球体、円柱などのプリミティブ形状を描画します。当たり判定のデバッグ表示等に便利です。
```cpp
auto primitiveObj = std::make_shared<GameObject>("Cube");
auto* primitive = primitiveObj->AddComponent<PrimitiveRendererComponent>();
primitive->SetPrimitiveType(PrimitiveType::Cube);
primitive->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f });
```

#### SpriteRendererComponent (2Dスプライト)
画面にUIなどの2D画像を表示します。
```cpp
auto spriteObj = std::make_shared<GameObject>("TitleLogo");
auto* sprite = spriteObj->AddComponent<SpriteRendererComponent>();
sprite->LoadTexture("ui/title_logo.png"); // resources/ 以下のパス
sprite->SetAnchorPoint({ 0.5f, 0.5f });
// ※ポストプロセス（ブルーム等）の影響を受けない最前面UIとして描画したい場合
sprite->SetTopMost(true);
```

### 2.2 カメラとライト (Camera & Lights)
シーンの視点や照明は以下の方法で管理されます。
- **`CameraComponent`**: 視点と投影行列を管理します。GameObjectにアタッチして使用します。
- **ライト管理 (Directional/Point/Spot/Area)**: ライトはコンポーネントとしてではなく、`BaseScene` が直接管理します。デバッグや調整を行う場合は、エディタのデバッグタブ「Camera & Lights」から各パラメータを直接編集できます。

### 2.3 物理・当たり判定 (Colliders & Raycast)
3D空間での衝突判定には、以下のコライダーコンポーネントを使用します。
- **`AABBColliderComponent` / `OBBColliderComponent`**: ボックス形状での当たり判定（軸平行 または 有向境界箱）。
- **`SphereColliderComponent`**: 球体形状での当たり判定。
- **`RaycastComponent`**: 指定した方向へレイ（光線）を飛ばし、オブジェクトとの交差判定を行います。

```cpp
auto obj = std::make_shared<GameObject>("PlayerCollider");
auto* collider = obj->AddComponent<OBBColliderComponent>();
collider->SetSize({ 1.0f, 2.0f, 1.0f });
```
*(※衝突時に処理を行いたい場合は、後述の `OnCollisionEnter` コールバックを利用します)*

### 2.4 手動生成の描画クラス (Manual Rendering Classes)
※ 以下のクラス群は現在も使用可能ですが、基本的にはコンポーネント版（PrimitiveRendererComponent等）の使用が推奨されています。

#### 背景描画 (`Skybox`)
3D空間の全天球背景（空など）を描画します。
```cpp
std::unique_ptr<Skybox> skybox_ = std::make_unique<Skybox>();
// resources/ 以下のテクスチャ（.dds等のキューブマップ形式が推奨）を指定して初期化
skybox_->Initialize("skybox/sky.dds"); 

// 毎フレームの更新と描画
skybox_->Update();
skybox_->Draw();
```

#### プリミティブ形状 (`Primitive3DObject`, `LineClass`)
当たり判定のデバッグ表示や、プロトタイプの作成に便利な組み込み図形です。
以前は `CubeClass` や `SphereClass` などの専用クラスに分かれていましたが、現在は `Primitive3DObject` に統合されており、１つのクラスで複数の形状（Cube, Sphere, Cylinder, Plane, Torus 等）を自由に切り替えて表示できます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<Primitive3DObject> primitive_;
std::unique_ptr<LineClass> line_;

// 2. 初期化 (Initialize)
primitive_ = std::make_unique<Primitive3DObject>();
// (PrimitiveType::Cube などを指定して初期化)
primitive_->Initialize(PrimitiveType::Cube);
// 形状固有のパラメータ変更 (例：Cubeの場合は不要だがSphereならRadiusを変える等)
primitive_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f }); // 赤色で半透明

line_ = std::make_unique<LineClass>();
line_->Initialize();
line_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色の線

// 3. 更新と描画 (Update & Draw)
// 位置を更新（内部で自動的に isDirty = true がセットされます）
primitive_->SetPosition(playerPos);
primitive_->Update();
primitive_->Draw();

line_->SetStartAndEnd(startPos, endPos);
line_->Update();
line_->Draw(); // ライン専用のキューに登録される
```

#### 2D図形の描画 (`Primitive2DObject`, `Primitive2DBatch`)
2D画面上（UIやHUDなど）で図形を描画するためのクラスです。用途に応じて2種類のクラスを使い分けます。

- **`Primitive2DObject` (柔軟な単体描画)**
  頂点情報（サイズやピボット）を動的に変更したり、単体で細かく制御したい場合に使用します。
  ```cpp
  std::unique_ptr<Primitive2DObject> primitive2D_;
  primitive2D_ = std::make_unique<Primitive2DObject>();
  primitive2D_->Initialize(Primitive2DType::Circle);
  primitive2D_->SetColor({ 0.0f, 0.5f, 1.0f, 1.0f });
  primitive2D_->SetSize({ 100.0f, 100.0f }); // ピクセルサイズ相当
  
  // 更新と描画
  primitive2D_->SetPosition({ 640.0f, 360.0f, 0.0f });
  primitive2D_->Update();
  primitive2D_->Draw();
  ```

- **`Primitive2DBatch` (高速な大量描画 / インスタンシング)**
  弾幕やパーティクル表現など、同じ形状の図形を大量に描画する場合に使用します。GPUインスタンシングにより描画負荷を大幅に削減できます。
  ```cpp
  std::unique_ptr<Primitive2DBatch> batch2D_;
  batch2D_ = std::make_unique<Primitive2DBatch>();
  batch2D_->Initialize(Primitive2DType::Rect);
  
  // 描画したい数だけインスタンスを追加
  for (int i = 0; i < 100; ++i) {
      batch2D_->AddInstanceData({
          Vector3(10.0f * i, 100.0f, 0.0f), // 座標
          Vector3(0.0f, 0.0f, 0.0f),        // 回転
          Vector3(10.0f, 10.0f, 1.0f),      // スケール
          Vector4(1.0f, 1.0f, 1.0f, 1.0f)   // カラー
      });
  }
  
  // 更新と描画
  batch2D_->Update();
  batch2D_->Draw();
  ```

※ **内部データ構造の変更について**：
これまで使われていた `MeshModule` や `MaterialModule` は、それぞれ `MeshDesc` と `MaterialDesc` に名称変更され、`Renderer/Data/RenderData.h` に統合されています。描画パイプラインのコードを独自にカスタマイズする際はこの変更にご注意ください。

#### パーティクル (`GPUParticleSystem`)
コンピュートシェーダを利用して数万個のパーティクルを高速に描画するシステムです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<GPUParticleSystem> gpuParticle_;

// 2. 初期化 (Initialize)
gpuParticle_ = std::make_unique<GPUParticleSystem>();
gpuParticle_->Initialize("effect/particle_tex.png");
gpuParticle_->SetColor({ 1.0f, 0.5f, 0.1f, 1.0f });
gpuParticle_->SetParticleLife(0.5f, 1.0f); // 寿命(最小, 最大)

// 3. 放出設定と更新 (Update)
// 発生源の位置、進行方向、広がり、速度、拡散、1フレームの発生数
gpuParticle_->SetBeamEmitter(position, direction, 1.0f, 0.5f, 0.1f, 100);
gpuParticle_->SetEmit(true); // 放出ON
gpuParticle_->Update();

// 4. 描画 (Draw)
gpuParticle_->Draw();
```

### 2.5 コンポーネントシステム (Component System)
`ObjClass` などの単体クラスに代わる、モダンなオブジェクト構築手法です。`GameObject` に必要な `Component` を組み合わせて機能を構築します。

```cpp
// 1. GameObjectの生成
auto object = std::make_shared<GameObject>("MyEntity");

// 2. コンポーネントのアタッチ
auto* transform = object->AddComponent<TransformComponent>();
auto* renderer = object->AddComponent<MeshRendererComponent>();

// 3. パラメータ設定
transform->position_ = {0, 10, 0};
renderer->LoadModel("enemy/boss.obj");

// 4. シーンへの登録（SceneManager経由で管理する場合）
// scene->AddGameObject(object);
```

#### 新規コンポーネント作成時のルール（自動UI化とメニュー登録）
エンジンの「簡易リフレクション」と「動的メニュー生成」により、コンポーネントを追加する際の手間が大幅に削減されています。
変数を宣言して `OnRegisterProperties` に書くだけで、**自動でインスペクターにUIが作成され、JSONに保存・復元される**ようになります。手動で ImGui を書く必要はありません。
さらに、作成したクラスを `ComponentFactory` に登録するだけで、エディタの「Add Component」メニューにも自動で追加されます（エディタのソースコードを変更する必要はありません）。

```cpp
#pragma once
#include "Framework/Component/Component.h"

class PlayerStatusComponent : public Component {
public:
    int hp_ = 100;
    float speed_ = 5.0f;

    // 1. コンポーネント名を返す
    // ※この名前に特定のキーワードを含めることで、Add Component メニューのカテゴリが自動で決まります。
    // （例："Renderer" や "Emitter" -> Renderer枠, "Collider" -> Collider枠, "UI", "Button" -> UI枠）
    std::string GetComponentName() const override { return "PlayerStatusComponent"; }

    // 2. 変数をリフレクションシステムに登録する
    void OnRegisterProperties() override {
        RegisterProperty("Max HP", &hp_);
        RegisterProperty("Move Speed", &speed_);
    }
};
```
*※作成したコンポーネントは、必ず `ComponentFactory.cpp` の `RegisterAllCoreComponents` 関数内（またはゲーム側の初期化処理）でファクトリに登録してください。*
```cpp
// 登録例
ComponentFactory::Register("PlayerStatusComponent", []() { return std::make_shared<PlayerStatusComponent>(); });
```

#### 衝突判定とコールバック (OnCollisionEnter / Destroy)
ゲームロジックとして「何かにぶつかったら壊れる」「ダメージを受ける」といった処理は、Component 内の仮想関数をオーバーライドして実装します。

```cpp
#include "Framework/Component/Component.h"
#include "Framework/GameObject.h"

class BulletComponent : public Component {
public:
    std::string GetComponentName() const override { return "BulletComponent"; }

    // 当たり判定が行われ、他のコライダーと接触した瞬間に自動で呼ばれます
    void OnCollisionEnter(GameObject* hitObject) override {
        // 相手の名前やComponentを見て処理を分ける
        if (hitObject->GetName() == "Enemy") {
            // Destroy() を呼ぶと、現在のフレームの終わりに安全にオブジェクトが破棄(GC)されます
            GetGameObject()->Destroy();
        }
    }
};
```

#### ポーズ（一時停止）時の動作制御
エディタのデバッグ機能としてのPauseボタン押下時（`TimeScale == 0.0f`時）は、すべてのコンポーネントの `Update()` 呼び出しが**フレームワーク側で自動的にスキップ**されます。そのため、各コンポーネント内で `deltaTime <= 0.0f` をチェックして手動で停止させる処理を書く必要はありません。

もし「UIのアニメーション」や「エフェクト」など、**ポーズ中であっても更新し続けたい**特殊なコンポーネントを作成する場合は、`CanUpdateWhenPaused()` 仮想関数をオーバーライドして `true` を返すようにしてください。

```cpp
class AlwaysMovingComponent : public Component {
public:
    std::string GetComponentName() const override { return "AlwaysMovingComponent"; }

    // ポーズ中（TimeScale == 0.0f）でもUpdateを実行するかどうか
    bool CanUpdateWhenPaused() const override { 
        return true; 
    }

    void Update() override {
        // ポーズ中も呼ばれる。
        // ※この中で GetGameDeltaTime() を使うと 0 になるため、
        //   ポーズ中も動かしたい場合は GetRealDeltaTime() など実時間を使う必要があります。
    }
};
```

#### 動的生成 (Instantiate)
弾を撃つ、敵を出現させるといった「ゲームプレイ中にオブジェクトを生み出す」処理は、あらかじめ作成したプレハブ（JSON）を指定して呼び出します。

```cpp
#include "Framework/Component/Component.h"
#include "Framework/GameObject.h"

class PlayerShooterComponent : public Component {
public:
    std::string GetComponentName() const override { return "PlayerShooterComponent"; }

    void Update() override {
        // 例: 左クリックされたら弾を生成する
        if (engine_->GetInputManager()->IsMouseButtonPressed(Mouse::Button::Left)) {
            // 現在の座標を取得
            Vector3 pos = GetGameObject()->GetComponent<TransformComponent>()->position_;
            
            // "resources/scenes/Bullet.json" をもとに、指定した座標へオブジェクトを生成
            auto bullet = GetGameObject()->Instantiate("Bullet", pos);
        }
    }
};
```

#### 演出・エフェクトコンポーネント (AudioSource / ParticleEmitter / VoxelParticle)
BGMやSEを鳴らしたり、エフェクトを発生させるには、インスペクターからプロパティを設定するだけで動く以下のコンポーネントが便利です。

- **`AudioSourceComponent`**: アタッチしたオブジェクトから音を鳴らします。
  - `Audio Path`: 再生したい音声ファイルのパス（例: `audio/se/boom.wav`）。
  - `Play On Awake`: シーン開始時に自動で再生するか。
  - `Loop`: ループ再生するか。
- **`ParticleEmitterComponent`**: アタッチしたオブジェクトの位置でエフェクトを発生させます。
  - `Texture Path`: パーティクルに使う画像パス。
  - `Emit Type`: 0(球体), 1(ビーム), 2(ボックス), 3(円柱) などの放出形状。
  - `Color`, `Velocity`, `Emit Count` などで自由にエフェクトを構築できます。
- **`VoxelParticleComponent`**: 指定した3Dモデルをボクセル化し、大量のパーティクルとして爆発（四散）させる高度なエフェクトコンポーネントです。
  - `Override Model Name`: ベースとなるモデルのファイル名を指定します。空欄の場合はアタッチされているRendererのモデルを使用します。
  - `Pre Allocate Count`: シーンロード時に事前計算・プールしておく数を指定し、再生時の処理落ちを防ぎます。
  - `Resolution`: ボクセルの分割数（例：32x32x32）。
  - C++コードから `GetComponent<VoxelParticleComponent>()->Explode();` を呼ぶことで破砕エフェクトが起動します。

#### UIコンポーネント (Canvas / Button / Text)
ゲーム内の2D UIを構築するための専用コンポーネントです。

- **`TextRendererComponent`**: 高品質なMSDF（マルチチャンネル符号付き距離場）フォントを利用し、任意の文字列を描画するコンポーネントです。動的に必要な文字だけを生成・パッキングするため、メモリ効率が非常に高く、どれだけ拡大しても文字がジャギりません（ぼやけません）。
  - `Text`: 表示する文字列。複数行の入力（改行）にも対応しています。
  - `Font ID`: 使用するフォント名（`resources/fonts/` 以下に配置したファイル名、例：`toro_glitch`）。
  - `Base Scale`: 文字の大きさ。
  - `Color`: 文字の色と透明度。
  - `Top Most`: ブルームなどのポストプロセスの影響を受けない、最前面レイヤーに描画するかどうか。
  - `Alignment`: テキストのアライメント（`0=Left`, `1=Center`, `2=Right`）。複数行の場合は行ごとに適用されます。

- **`ButtonComponent`**: マウスカーソルのホバーやクリックを検知し、色を自動的に変えたり、明滅（Pulse）アニメーションを行うUI機能を提供します。
  - `Normal/Hover/Click Color`: マウスの操作状態に合わせて、アタッチされているSpriteの色を自動的に変化させます。
  - `Enable Hover Pulse / Idle Pulse`: ホバー時や待機時の明滅アニメーションを有効化します。
  - *(※ボタン押下によるシーン遷移機能はエンジン側の責務ではなく、`Application/` 側の `SceneTransitionButtonComponent` などで個別に実装してアタッチしてください)*
- **`CanvasComponent`**: UI要素をグループ化し、アルファ値（透明度）などを一括管理します。
  - `Group Alpha`: このコンポーネントを持つGameObject自身と、そのすべての子要素にある `SpriteRendererComponent` のアルファ値を一括で制御します。フェードイン・フェードアウトの演出に便利です。
### 2.6 汎用エフェクトシステム (`Effect`) と 3D爆発エフェクト (`kExplosion`)

敵や障害物に弾丸・ミサイルが着弾した際に使用するリッチなエフェクト機能です。`Effect` クラスは `EffectType` 列挙型により複数の表現をサポートしています。

#### サポートされているエフェクトの種類 (`EffectType`)
- **`kHit`**: ヒットエフェクト（星型に広がる斬撃など）
- **`kImpact`**: 衝撃エフェクト（PlaneとRingの複合ヒット表現）
- **`kAura`**: キャラクターを包むオーラエフェクト
- **`kSwing`**: 武器を振った際の軌跡（風切り）エフェクト
- **`kExplosion`**: 3D爆発エフェクト（球体膨張＋火花＋衝撃波）

#### プレイヤーでの事前生成とプール管理の例

弾丸が連射されたり、同時に多数ヒットした場合に備え、事前にエフェクトオブジェクトをプールしておき使い回す設計が推奨されます。

```cpp
// --- ヘッダー (Player.h) ---
#include "Renderer/Effect/Effect.h"
#include <vector>
#include <memory>

class Player {
private:
    std::vector<std::unique_ptr<Effect>> explosionEffects_;
    static const int kMaxExplosionEffects = 32;
public:
    void Initialize(InputManager* input, IrufemiEngine* engine);
    void Update();
    void Draw();
    void PlayExplosion(const Vector3& position);
};

// --- 実装 (Player.cpp) ---
void Player::Initialize(InputManager* input, IrufemiEngine* engine) {
    // プールを事前に生成・初期化
    explosionEffects_.clear();
    for (int i = 0; i < kMaxExplosionEffects; ++i) {
        auto effect = std::make_unique<Effect>();
        effect->Initialize(EffectType::kExplosion);
        explosionEffects_.push_back(std::move(effect));
    }
}

void Player::Update() {
    // アクティブなエフェクトのみ毎フレーム更新
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->Update();
        }
    }
}

void Player::Draw() {
    // アクティブなエフェクトの描画・同期
    for (auto& effect : explosionEffects_) {
        if (effect->IsActive()) {
            effect->SyncBeforeDraw();
            effect->Draw();
        }
    }
}

void Player::PlayExplosion(const Vector3& position) {
    // プールから空いているエフェクトを探して再生開始
    for (auto& effect : explosionEffects_) {
        if (!effect->IsActive()) {
            effect->Play(position);
            break;
        }
    }
}
```

#### 着弾時の呼び出し例 (GameScene.cpp など)

```cpp
if (Collision::IsOBBSphereCollision(part->GetOBB(), bulletSphere)) {
    bullets[i].isActive = false;
    // 着弾位置に爆発エフェクトを発生
    player_->PlayExplosion(bullets[i].position);
}
```

### 2.7 Data-Oriented Design (DOD) と ComponentPool

コンポーネントシステムにおいて、同じ種類のコンポーネントを連続したメモリ空間（プール）に配置し、CPUキャッシュヒット率を劇的に向上させるための最適化の仕組みです。
何万個もの弾やパーティクル、多数の敵を同時に処理する場合に、標準の `std::make_shared` によるメモリの断片化を防ぎます。

#### プール対応コンポーネントの作り方
特定のコンポーネントを `ComponentPool` の管理下に置くには、対象のコンポーネントクラスの宣言の下で `IsPooledComponent` のテンプレート特化を行います。

**例: `TransformComponent.h` の場合**
```cpp
#pragma once
#include "Component.h"
#include "Engine/Core/System/ComponentPool.h"

class TransformComponent : public Component {
    // ... 通常のコンポーネント実装 ...
};

// ComponentPool 対応を宣言（ファイルの末尾に記述）
template<> struct IsPooledComponent<TransformComponent> : std::true_type {};
```

この一行を追加するだけで、`GameObject::AddComponent<TransformComponent>()` や、エディタ・JSONからの自動ロード（`ComponentFactory`）が**すべて自動的にプール経由での生成**に切り替わります。

#### DODの恩恵を最大限に受ける一括更新処理
プール化されたコンポーネントは、`ComponentPool::ForEach` を使って全インスタンスを一気に処理（バッチ処理）することができます。
現在、`TransformComponent` の座標行列計算はこの機能を用いて `BaseScene::Update` 内で毎フレーム最初に一括で計算 (`TransformComponent::UpdateAll()`) されています。
これにより、何万ものオブジェクトの Transform 行列計算がキャッシュミスなしで爆速で行われるようになっています。

```cpp
void TransformComponent::UpdateAll() {
    currentFrame_++;
    ComponentPool<TransformComponent>::GetInstance().ForEach([](TransformComponent& transform) {
        transform.ComputeMatrix();
    });
}
```
※注意: プール対応にしたコンポーネントは、ゲーム終了時にプールから安全にメモリ解放されます。ユーザー側で特別なメモリ管理コード（`delete`など）を書く必要はありません。

### 2.8 カスタムパラメータの渡し方 (Custom Constant Buffer)
エンジン標準の `Material` には含まれない独自のパラメータ（演出用の色やアニメーションフラグなど）をシェーダーに渡したい場合、エンジンの `Material` を汚染するのではなく、専用の定数バッファ枠 (`RootSlot::Special` / レジスタ `b6`) を利用します。

```cpp
// 1. 専用の構造体と定数バッファを定義 (16バイトアライメントを意識)
struct MyCustomParams {
    Vector4 customColor;
    float param1;
    float padding[3];
};

std::unique_ptr<DynamicConstantBuffer<MyCustomParams>> myCb_;
MyCustomParams params_{};

// 2. 初期化時にバッファを生成し、描画オブジェクトにGPUアドレスをセット
myCb_ = std::make_unique<DynamicConstantBuffer<MyCustomParams>>();
myCb_->Initialize(engine_->GetDXCommon(), 1);
myCb_->Update(params_);

model_->GetResource()->SetCustomCBVAddress(myCb_->GetGPUVirtualAddress());

// 3. HLSL (シェーダー) 側で受け取る
// ConstantBuffer<MyCustomParams> gMyParams : register(b6);
```

### 2.9 マルチバッファ同期と基底クラス (`MultiBufferSyncState`)
DirectX 12 でフレーム間のマルチバッファリング（`kMaxFramesInFlight`）を行う際、CPUからGPUへの定数バッファの更新タイミングを管理するために `MultiBufferSyncState` 基底クラスを利用します。
`BaseResource` や `BaseModel` などの描画リソースクラスは、すでにこのクラスを継承しています。

#### 使い方ルール
1. **CPU側でのデータ変更時 (`Update` 等)**
   描画オブジェクトのパラメータ（座標、色、カスタムマテリアル等）を変更した際は、必ず `MarkAsDirty()` を呼び出します。これにより、全フレームバッファ（最大3フレーム分）に「更新が必要」というフラグが立ちます。
   ```cpp
   void Skybox::Update() {
       // 行列や色の計算...
       transformationMatrix_.World = worldMatrix;
       
       // データが変更されたので全フレームバッファのDirtyフラグを立てる
       MarkAsDirty();
   }
   ```
2. **GPUバッファへの同期時 (`SyncBeforeDraw` 等)**
   GPUにデータを書き込む直前（描画の直前）で `CheckAndClearDirty(frameIndex)` を呼び出します。この関数は、対象フレームの更新フラグが立っている場合のみ `true` を返し、同時にフラグをクリアします。
   ```cpp
   void Skybox::SyncBeforeDraw() {
       uint32_t frameIndex = engine_->GetDrawManager()->GetDxCommon()->GetFrameIndex();
       
       // 対象フレームのバッファが古い場合のみ、GPUへ転送する
       if (CheckAndClearDirty(frameIndex)) {
           transformationBuffer_.Update(transformationMatrix_, frameIndex);
       }
   }
   ```
   ※従来の手動配列管理（`isDirtyBuffer_[frameIndex] = true` 等）は非推奨です。

---

## 3. リソース管理 (Resource Management)

ゲームに必要なテクスチャ、3Dモデル、サウンドデータは、エンジン内の各 Manager を通して一元管理（ロード・キャッシュ）されます。

### 3.1 TextureManager (テクスチャ管理)
`Sprite` などの描画に必要な画像をロードし、GPU用のハンドルを取得します。同じ画像を何度ロードしても、1度だけメモリに乗るようになっています。

```cpp
// 1. 画像のロード (resources/ フォルダからの相対パス)
engine_->GetTextureManager()->Load("ui/title_logo.png");

// 2. フォルダ内の画像一括ロード (ローディング画面などで使用)
engine_->GetTextureManager()->LoadAllFromFolder("resources/ui");

// ※ Sprite等の Initialize にファイル名を渡せば、内部で自動的にロードされます。
```

### 3.2 ModelManager (3Dモデル管理)
`.obj` や `.gltf` 形式の3Dモデルを読み込み、最適化してキャッシュします。

```cpp
// モデルの事前ロード（インゲーム開始前などに呼ぶとカクつきを防げます）
engine_->GetModelManager()->LoadModel("enemy/enemy.obj");

// ※ ObjClass::Initialize("enemy/enemy.obj") を呼べば内部で自動ロードされます。
```

### 3.3 AudioManager (サウンド管理)
BGMやSEの再生・停止・音量調節を行います。

```cpp
// 1. サウンドの事前ロード
engine_->GetAudioManager()->GetOrLoadSoundByFile("resources/audio/bgm_title.wav");

// 2. フォルダごとの一括ロード (カテゴリ分け)
engine_->GetAudioManager()->LoadSoundsFromFolder("resources/audio/se", "SE");

// 3. 再生
auto soundData = engine_->GetAudioManager()->GetOrLoadSoundByFile("resources/audio/bgm_title.wav");
// (サウンドデータ, ループフラグ, 音量 0.0f~1.0f)
std::weak_ptr<VoiceInstance> voice = engine_->GetAudioManager()->Play(soundData, true, 0.8f);

// 4. 停止
engine_->GetAudioManager()->Stop(voice);
```

### 3.4 FontManager (フォントとテキストの管理)
TTF等のフォントファイルを読み込み、テキスト描画用の高品質なMSDF画像を動的に生成・キャッシュします。

```cpp
// 1. フォルダ内の全フォント一括ロード
engine_->GetFontManager()->LoadAllFromFolder("resources/fonts");

// 2. 【超重要】スパイク（処理落ち）防止のための事前生成
// ゲーム中に「まだ生成されていない全く新しい文字（複雑な漢字など）」が出現すると、
// メインスレッドでMSDF画像生成が行われ、一瞬ゲームがカクつく（スパイクする）可能性があります。
// プレイスルーを滑らかにするため、シーン開始時のロード中などに「使う予定の全文字」を事前生成してください。
engine_->GetFontManager()->PrecacheText("my_font", L"このシーンで使う予定の会話テキストや、よく使う漢字一覧など...");
```


---

## 4. 入力・演算・ユーティリティ (Input & Utility)

プレイヤーの操作を受け取ったり、ゲームに必要な計算を行うクラス群です。

### 4.1 入力システム (InputManager / InputMappingContext)
プレイヤーからの入力を取得する方法は、従来からの「直接キー・ボタンを指定する方法」と、より柔軟な「アクションバインディング」を利用する方法の2つがあります。

#### 1. 新しいアクションバインディング (推奨)
キーボードやゲームパッドの入力を「Jump」や「Attack」などの論理的なアクションにマッピングするシステムです。
複数の入力デバイスを一元管理したり、キーコンフィグの変更に対応しやすくなります。

```cpp
// 1. InputManager に直接アクションと物理入力をバインドする (初期化時など)
// 引数: アクション名, デバイスに対応したInputId (Keyboard_Space, Gamepad_A 等)
engine_->GetInputManager()->BindAction("Jump", InputId::Keyboard_Space);

// 2. アクションの状態を取得 (毎フレームのUpdate内など)
if (engine_->GetInputManager()->IsActionTriggered("Jump")) {
    // ジャンプ処理
}

// アナログ値の取得（スティック移動など）
auto moveVal = engine_->GetInputManager()->GetActionValue("Move");
float moveX = moveVal.Get<float>();
```

#### 2. 直接キー・ボタンを取得する方法
キーボード、マウス、ゲームパッド（XInput互換）の操作を直接取得します。
`BaseScene` を継承したクラスでは、直接 `PressedVK()` や `IsButtonPressed()` などのヘルパー関数を呼び出すのが最も簡単です。

```cpp
// 【キーボード (BaseScene内での使用例)】
// Spaceキーが「押された瞬間」か？ (VK: 仮想キーコード)
if (PressedVK(VK_SPACE)) { ... }
// Wキーが「押され続けている」か？ (DIK: DirectInput互換キーコード)
if (DownDIK(DIK_W)) { ... }

// 【ゲームパッド (BaseScene内での使用例)】
// Aボタンが押された瞬間か？
if (IsButtonPressed(XINPUT_GAMEPAD_A)) { ... }

// 左スティックの入力値 (-1.0f 〜 1.0f) ※入力値取得は InputManager 経由で行います
auto input = engine_->GetInputManager();
float lx = input->GetLeftStickX();
float ly = input->GetLeftStickY();

// 【マウス (InputManager経由)】
// 左クリックされたか？
if (input->IsMouseButtonPressed(Mouse::Button::Left)) { ... }
// マウスの現在座標 (スクリーン座標)
Vector2 mousePos = input->GetMousePosition();
```

### 4.2 Math ユーティリティ (算術演算)
`Math` 名前空間に関数がまとまっています。`Vector3` や `Matrix4x4` を安全に計算します。

```cpp
#include "Core/Math/Math.h"

Vector3 playerPos = { 0.0f, 0.0f, 0.0f };
Vector3 enemyPos  = { 10.0f, 0.0f, 10.0f };

// 距離の計算
Vector3 diff = Math::Subtract(enemyPos, playerPos);
float distance = Math::Length(diff);

// 正規化 (方向ベクトルの取得)
Vector3 direction = Math::Normalize(diff);

// 加算と乗算
Vector3 offset = Math::Multiply(5.0f, direction);
Vector3 targetPos = Math::Add(playerPos, offset);
```

### 4.3 Collision (当たり判定)
`OBB` (有向境界箱) や `Sphere` などの定義と交差判定を行います。

```cpp
#include "Core/Math/Geometry/OBB.h"
#include "Core/Math/Geometry/Sphere.h"

// 衝突判定の実装例
if (IsCollision(playerOBB, enemySphere)) {
    // プレイヤーと敵の球が当たった時の処理
}
```

### 4.4 UIメニュー構築の定石 (UISelectionGroup)
タイトル画面やポーズメニューなど、「上下で選んで決定する」UIを作るための便利なクラスです。手動でカーソル管理や長押し防止を書く必要がなくなります。

```cpp
#include "Framework/Overlay/UISelectionGroup.h"

// 1. メンバ変数に定義
UISelectionGroup uiGroup_;

// 2. Initialize() で設定
// (要素数, 初期選択インデックス, 横並びか, 連続入力の間隔, ループするか)
uiGroup_.Initialize(3, 0, false, 0.2f, true); 

// 3. Update() で入力判定
uiGroup_.Update(engine_->GetInputManager());

if (uiGroup_.IsTriggered()) {
    // 現在選ばれている項目を取得
    int selected = uiGroup_.GetSelectedIndex();
    if (selected == 0) {
        // 「ゲーム開始」が選ばれた
    } else if (selected == 1) {
        // 「設定」が選ばれた
    }
}

// ※描画時は uiGroup_.GetSelectedIndex() に応じて、選ばれている項目の色やスプライトを変えます。
```

### 4.5 UIのアニメーション演出 (`UIAnimator`)
UIの「明滅」や「浮遊」といった数学的なアニメーション（サイン波ベース）を簡単に計算してくれる便利なユーティリティクラスです。自分で時間を管理して計算式を書く必要がなくなります。

```cpp
#include "Framework/UIAnimator.h"

// 1. 宣言
UIAnimator uiAnimator_;

// 2. 毎フレームの更新 (Update内)
uiAnimator_.Update(engine_->GetGameDeltaTime());

// 3. 値の取得と適用
// ダークソウル風のゆっくりとした明滅アルファ値 (基本値0.6, 振幅0.4, 速度3.0)
float alpha = uiAnimator_.GetPulseAlpha(0.6f, 0.4f, 3.0f);
sprite_->SetColor({1.0f, 1.0f, 1.0f, alpha});

// 浮遊する上下のオフセット値
float offsetY = uiAnimator_.GetFloatOffset(10.0f, 2.0f);
sprite_->SetPosition({ 640.0f, 360.0f + offsetY });

// 警告やダメージ時の高速点滅 (true/false)
if (uiAnimator_.GetFlashVisibility(40.0f)) {
    // 描画する
}
```

---

## 5. 高度な機能と拡張 (Advanced Features)

ゲームをさらにリッチにするためのポストプロセス設定や、デバッグ用のUI機能、およびエンジンを独自に拡張する際のお作法です。

### 5.1 ポストプロセス (PostProcessManager)
画面全体にさまざまなエフェクト（ブルーム、ビネット、ノイズ、ディゾルブなど）をかけます。複数のエフェクトをスタック（重ね掛け）することが可能です。

```cpp
auto* pp = engine_->GetPostProcessManager();

// 1. エフェクトのリセットと適用
pp->ClearActiveModes();
pp->AddActiveMode(PostProcessMode::Bloom);
pp->AddActiveMode(PostProcessMode::Vignette);

// 2. パラメータの調整
// ブルーム（発光）の強度を調整
pp->GetBloomParams().intensity = 1.2f;
// ビネット（画面端の暗転）の強さを調整
pp->GetVignetteParams().power = 0.8f;

// ※ シーン遷移時のフェードなどもこれを利用して実装できます
```

#### サポートされているポストプロセスモード
`PostProcessMode` 列挙型には、以下の多彩なエフェクトが用意されています。複数のモードを `AddActiveMode()` でスタックすることが可能です。
- **色調補正系**: `ToneMapping` (ACES露出補正), `Grayscale`, `Sepia`, `HSV`
- **空間・ぼかし系**: `Smoothing`, `GaussianFilter`, `RadialBlur` (放射状ぼかし)
- **画面演出系**: `Bloom` (発光), `Vignette` (暗転), `DepthBasedOutline` (アウトライン抽出), `Dissolve` (消失演出), `Noise`, `Glitch` (画面の乱れ)
- **画面遷移系**: `Fade`, `Slide`

### 5.2 デバッグ機能 (DebugUI / ImGui)
パラメータの調整や変数の確認を行うために、ImGuiを利用してデバッグウィンドウを表示できます。
`BaseScene` を継承したクラスでは、`DrawDebugTab()` をオーバーライドするだけで自動的にタブが追加されます。

```cpp
void ExampleScene::DrawDebugTab() {
#if defined USE_IMGUI
    // 親クラスの処理を呼ぶ
    BaseScene::DrawDebugTab();

    // 独自のタブを追加
    if (ImGui::BeginTabItem("Example Scene")) {
        // パラメータ調整用スライダー
        static float playerSpeed = 5.0f;
        ImGui::SliderFloat("Player Speed", &playerSpeed, 1.0f, 10.0f);

        // ボタンの配置
        if (ImGui::Button("Reset Player")) {
            playerObj_->SetPosition({0, 0, 0});
        }
        ImGui::EndTabItem();
    }
#endif
}
```

### 5.3 カスタム機能の追加ルール (エンジンの拡張)
エンジンに新しい機能を追加する際は、以下のルールを守ってください。

1. **アーキテクチャの分離**
   - エンジンコア (`IrufemiEngine/`) には、特定のゲームロジック（プレイヤークラスや固有のシーン）を含めないでください。
   - ゲームロジックは必ず `Application/` ディレクトリ配下に作成します。

2. **新しい描画表現 (RenderPass) の追加**
   - 独自のシェーダーや特殊な描画パイプラインを追加したい場合は、`DrawManager` に直接書くのではなく、`IRenderPass` を継承したクラスを作成し、`RenderGraph` に登録してください。
   - これにより、他の描画パスに影響を与えずに安全に機能を追加できます。

3. **ドキュメントの更新**
   - `Manager` の関数名を変えたり、全く新しい機能（例: ECSシステム）を導入した場合は、必ずこの `Manual.md` も合わせて更新し、チームメンバーがすぐに使えるようにしてください。

---

## 6. エディタ (IrufemiEngine Editor) の使い方

### Project Browser の新機能
IrufemiEngine のエディタは、モダンなゲームエンジン（Unity等）ライクなアセット管理をサポートしています。

1. **2ペイン・タイル表示**
   - **左ペイン (ツリー表示)**: フォルダの階層構造のみを表示します。クリックするとその中身が右ペインに表示されます。
   - **右ペイン (タイル表示)**: FontAwesome アイコン付きで、ファイルとフォルダがグリッド状に並びます。画面幅に合わせて自動で折り返されます。

2. **コンテキストメニュー (右クリック操作)**
   - ファイル/フォルダ上で右クリックすると、「**Rename (名前変更)**」や「**Delete (削除)**」が可能です。
   - 余白を右クリックすると、「**Create Folder (新規フォルダ作成)**」ができます。
   - ※これらの操作はOSの実ファイルシステム（Windowsのフォルダ等）に即座に反映されます。

3. **ドラッグ＆ドロップによるアセットの即時配置**
   - **画像の配置**: Project Browser にある `.png` や `.jpg` などの画像を **Hierarchy** にドラッグ＆ドロップすると、`SpriteRendererComponent` がアタッチされた GameObject が自動生成されます。
   - **3Dモデルの配置**: `.obj` や `.fbx` などをドラッグ＆ドロップすると、`MeshRendererComponent` がアタッチされた GameObject が自動生成されます。
   - D&D 時にアセットの相対パスが自動的にコンポーネントへセットされるため、すぐに画面上でプレビュー可能です。

### 6.2 プレハブ (Prefab) システム
Unityライクな「オブジェクトのテンプレート化」をサポートしています。

1. **Prefabの作成・保存**:
   - Hierarchy パネルでオブジェクトを右クリックし、「**Save as Prefab**」を選択します。
   - `resources/prefabs/` 以下に `.prefab.json` として、コンポーネントやパラメータがすべて保存されます。
2. **エディタからの配置**:
   - Project Browser に青いキューブのアイコンで表示されます。これを **Scene View** にドラッグ＆ドロップすると、即座にインスタンス化（実体化）して配置されます。
3. **C++コードからの動的生成 (ランタイム)**:
   - ゲーム中に「敵」や「弾」をスポーンさせるには、C++コードで以下のように呼び出します。
   ```cpp
   #include "Framework/SceneSerializer.h"

   // Prefabをロードしてインスタンス化
   auto bullet = SceneSerializer::LoadPrefab("resources/prefabs/Bullet.prefab.json");
   if (bullet) {
       bullet->GetComponent<TransformComponent>()->position_ = spawnPos;
       scene->AddGameObject(bullet);
   }
   ```

### 6.3 カスタムエディタUI構築と Undo/Redo (Ctrl+Z) 対応
エンジン標準の `GetProperties()` （簡易リフレクション）を使わずに、複雑なカスタムUIを構築したい場合は、`IComponentEditor` を継承したクラスを作成します（実装は `ComponentEditorRegistry.cpp` に記述）。
この際、インスペクターでの値の変更を Undo/Redo (Ctrl+Z) に対応させるため、以下の専用のヘルパー関数を使用してください。

**1. ドラッグ操作など連続する値の変更 (`CheckUndoRedoDrag`)**
`ImGui::DragFloat` や `ImGui::ColorEdit` など、ドラッグ中に値が連続的に変わるUIでは、操作の「開始」と「終了」のタイミングを自動判定してコマンドを発行する `CheckUndoRedoDrag` を呼び出します。

```cpp
// UI描画と値の更新
if (ImGui::DragFloat("Base Scale", &scale, 0.1f, 0.1f, 1000.0f)) {
    comp->SetBaseScale(scale); // ドラッグ中のリアルタイム反映
}
// ドラッグ終了時に自動でUndo/Redoコマンドを発行
CheckUndoRedoDrag(actionManager, &scale, [comp](const float& v) { 
    comp->SetBaseScale(v); 
});
```

**2. チェックボックスやコンボボックスなど即時確定する変更 (`PushInstantUndo`)**
`ImGui::Checkbox` や `ImGui::BeginCombo` など、クリックした瞬間に値が確定するUIでは、変更前後の値を使って `PushInstantUndo` を呼び出します。

```cpp
bool isTopMost = comp->IsTopMost();
if (ImGui::Checkbox("TopMost", &isTopMost)) {
    // 変更前の値、変更後の値、値をもとに戻すためのセッター関数（ラムダ）を渡す
    PushInstantUndo(actionManager, comp->IsTopMost(), isTopMost, [comp](const bool& v) { 
        comp->SetTopMost(v); 
    });
}
```

### 6.3.5 エディタのショートカットキー
エディタ上では、操作を快適にするために以下のショートカットキーがサポートされています。
- **`Ctrl + Z`**: 元に戻す (Undo)
- **`Ctrl + Y`**: やり直し (Redo)
- **`Ctrl + D`**: 選択中の GameObject を複製 (Duplicate)
- **`Delete`**: 選択中の GameObject を削除
- **`F2`**: 選択中の GameObject の名前変更 (Rename)
- **`W / E / R`**: ギズモの操作モード切り替え (Translate / Rotate / Scale)

### 6.3.6 コンソールパネル (Console)
エンジンからのログ出力（エラー、警告、情報）は、エディタ下部の **Console** パネルに表示されます。
コード内で `Log::Info()`, `Log::Warning()`, `Log::Error()` を呼び出すとリアルタイムに反映され、パネル上の「Clear」ボタンでログを消去できます。

### 6.4 インスペクター (Inspector) の便利な操作機能
インスペクター上で作業を効率化するための便利な機能が備わっています。これらはすべて **Undo/Redo (Ctrl+Z / Ctrl+Y)** に対応しています。

1. **コンポーネントの削除 (Remove Component)**
   - インスペクターに追加されている各コンポーネントの「ヘッダー（名前が書かれた帯の部分）」を **右クリック** すると、コンテキストメニューが表示されます。
   - そこから「**Remove Component**」を選択することで、不要なコンポーネントを安全に削除できます。
2. **Transform の一括リセット**
   - `TransformComponent` のヘッダーの右端にある「**Reset**」ボタンをクリックすると、Position (0, 0, 0)、Rotation (0, 0, 0)、Scale (1, 1, 1) へ一括で初期化されます。
3. **テクスチャのドラッグ＆ドロップ割り当て**
   - `SpriteRendererComponent` などのテクスチャ項目（コンボボックス等のUI）に対して、Project Browser から画像ファイル（`.png` や `.jpg` など）を直接 **ドラッグ＆ドロップ** することで、すぐにテクスチャを適用できます。

### 6.5 Hierarchy と Scene View の便利な操作機能
シーンが複雑になりオブジェクトが増えてきた場合、以下の機能を使って整理・保護や効率的な操作を行うことができます。

1. **インライン・リネーム (Inline Rename)**
   - Hierarchy 上でオブジェクトの名前部分を **ダブルクリック** すると、その場で名前を直接編集モード（テキスト入力）に入ることができます。Inspector を開かなくても素早い名前変更が可能です。
2. **可視アイコンとロックアイコン (Eye & Lock)**
   - **目のアイコン (可視性)**: Hierarchy の右端にある目のアイコン（👁/🚫）をクリックすると、オブジェクトの Active/Inactive を即座に切り替えられます。
   - **南京錠アイコン (保護)**: 鍵アイコン（🔒/🔓）をクリックすると編集がロックされます。ロックされたオブジェクトは以下の操作が制限されます：
     - Scene View 上でのギズモ操作の無効化（動かせなくなる）
     - Inspector 上での全パラメータ編集、コンポーネントの追加・削除の無効化（Read-Onlyになる）
     - ドラッグ＆ドロップによる階層移動の禁止（間違って他のオブジェクトの子にしてしまう事故を防ぐ）
3. **フォルダ機能 (Folder)**
   - Hierarchy 上の空白部分を **右クリック** し、「**Create Folder**」を選択すると、空のフォルダ（`isFolder_ = true` の GameObject）が作成されます。
   - 専用のアイコン（📂）が付き、他のオブジェクトを見やすくグループ化できます。フォルダは Scene View 上でギズモが非表示になります。
4. **Scene View のオーバーレイUI (Overlay UI)**
   - Scene View の右上に、半透明のオーバーレイパネルが常駐しています。
   - ここから、ギズモの操作モード（Local/World）、操作ツール（Translate, Rotate, Scale, Bounds）の切り替えや、コライダーのデバッグ表示のON/OFFが素早く行えます。

### 6.6 メニューバーの便利機能
画面最上部のメニューバーからも、様々なアクションにアクセスできます。

- **GameObject メニュー**: 
  - ここから即座にプリミティブオブジェクト（Cube, Sphere, 2D Spriteなど）や空のオブジェクトをシーンに追加できます。Hierarchyの右クリックメニューと同じ機能です。
- **Window メニュー**:
  - **Performance**: オンにすると、現在のフレームレート（FPS）やデルタタイム、フレームごとの処理時間（ms）を確認できる「Performance」ウィンドウが表示されます。ゲームの最適化時の確認に便利です。
  - **Layout**: UIレイアウトを初期状態に戻したり、現在の配置をデフォルトとして保存できます。
    - **Reset Layout**: パネルを誤って閉じてしまったり、配置がおかしくなった場合に、ドッキング状態を強制的に初期配置へ復元します。

### 6.7 Play / Pause コントロールとゲーム時間の管理
メニューバーの中央にあるコントロールから、エディタ上でのゲームの進行を制御できます。

1. **Play / Stop (▶ / ■)**
   - **Play**: Editモードからゲームを実行状態（Playモード）に移行します。移行した瞬間に現在のシーン状態が自動でバックアップされ、**Stop** を押すと変更が破棄されて Editモードの初期状態に完全にリセットされます。
2. **Pause (⏸)**
   - ゲーム実行中に時間を一時停止します。
   - ポーズ中はゲーム内の `GetDeltaTime()` が常に `0.0f` を返すようになるため、キャラクターの移動や物理演算などの更新処理がすべて止まります。
   - 一方、エディタのUIやカメラ操作などは実時間である `GetRealDeltaTime()` を使用しているため、ゲームが止まっている状態でもシーンを自由に観察したり、Inspectorからパラメータを調整することが可能です。

---
## 7. トラブルシューティング (Troubleshooting)

### 7.1 アプリケーション終了時に `LIVE_DEVICE` エラーでクラッシュする
**現象**: Visual Studio の出力ウィンドウに `D3D12 WARNING: Live ID3D12Device` と表示され、`D3DResourceLeakChecker` でブレークポイントが止まる。

**原因**: 
- `PrimitiveManager` などのシングルトンクラスが GPU リソース（頂点バッファ等）を保持したまま、DirectX のデバイスが破棄されようとした際に発生します。
- または、`RenderTexture` や `DescriptorPool` から確保したリソースが解放されていない場合や、`TransientResourceManager` 等でフレーム終了時の遅延破棄(`ReleaseAfterFence`) キューに積まれたリソースがアプリケーション終了時にクリアされていない場合に発生します。
- 注意点として、エラー追跡用に追加した `ID3D12InfoQueue` などの COMポインタ自体のスコープが長すぎて、`ReportLiveObjects` の時点でデバイスの参照を保持してしまう（見かけ上のリーク）こともあります。

**解決策**:
- `IrufemiEngine::Finalize()` 内で、`dxCommon_` が破棄される前に、すべてのマネージャーの `Finalize()` または `reset()` を呼ぶようにしてください。シングルトンの場合は `PrimitiveManager::Finalize()` のように明示的に呼び出します。
- **フレーム遅延破棄の注意**: `dxCommon_->ReleaseAfterFence(resource)` で破棄を予約したリソースは、`DirectXCommon::pendingResources_` に保持されます。エンジン終了時には、必ずGPU同期待ち（`WaitForGPU()`）の直後に `pendingResources_.clear()` を呼び出して完全に破棄してください。
- COMポインタ（`ComPtr`）を使用する場合は、不要になったら `Reset()` を呼ぶか、寿命を強制的に限定するためローカルスコープ `{}` 内で宣言するようにしてください。
---

## 8. シェーダーの追加・変更とコンパイル構成 (Shaders & Compilation)

本エンジンでは、パフォーマンスと開発効率の両立のため、ビルド構成（Configuration）によってシェーダーのコンパイル方式が完全に切り替わる「ハイブリッド構成」を採用しています。

### 8.1 開発時のホットリロード (Editor / Debug ビルド)
Editor または Debug モードで起動している場合、ゲームを実行したまま（エディタを立ち上げたまま）、シェーダーファイル（`.hlsl` または `.hlsli`）を上書き保存するだけで、自動的に **ホットリロード** が行われます。
- バックグラウンドでフォルダを監視しており、保存を検知すると安全に古いシェーダーとPSOキャッシュを破棄し、再コンパイルして即座に描画に反映します。
- 一々ビルドし直す必要がないため、ライティングの微調整やエフェクト作成のイテレーションが非常に高速です。

### 8.2 シェーダーの追加手順と命名規則
新しいシェーダー（`.hlsl`）を追加する際は、必ずファイル名の中に「プロファイル名」を含めるようにしてください。ビルド時の自動コンパイルバッチ（`CompileShaders.bat`）がこのファイル名を見て適切なコンパイルを行います。

- **頂点シェーダー**: `xxx.VS.hlsl`
- **ピクセルシェーダー**: `xxx.PS.hlsl`
- **コンピュートシェーダー**: `xxx.CS.hlsl`
- **ジオメトリシェーダー**: `xxx.GS.hlsl`

### 8.3 配布環境 (Release ビルド) と CSOファイル
Releaseビルドでは、`dxcompiler.dll` などのコンパイラを一切ロードせず、最速で起動させる仕組みになっています。
- Visual Studio で F5（またはビルド）を押した直後に、裏で自動的に `CompileShaders.bat` が走り、すべての `.hlsl` をコンパイルして `.cso`（コンパイル済みバイナリ）を生成します。
- 実行時には `.hlsl` ではなく生成された `.cso` を直接メモリに読み込みます。
- そのため、最終的にプレイヤーに配布（リリース）する際は、**「すべての `.hlsl` / `.hlsli` ファイルは削除して `.cso` だけを含める」** ことで、ソースコードの秘匿化と容量削減が可能です（Compute Shaderも含む）。

---
> **ドキュメント更新履歴**
> - 2026/05: RenderGraph / パケット分離対応を反映、Phase 1~3 および 便利機能（カメラ、ディレクトリ構造、UISelectionGroup）を追記
> - 2026/05: エディタ機能（Project Browserの2ペイン化、FontAwesome対応、ドラッグ＆ドロップ機能）の解説を追記
> - 2026/05: コンポーネントシステム（GameObject/Component）の導入と、UIロジックの分離（Registry方式）を追記
> - 2026/05: 高品質テキスト描画システム (`TextRendererComponent` / `FontManager`) と、Play時の自動保存機能を追記
> - 2026/05: 終了時のリソースリーク（LIVE_DEVICE）対策とトラブルシューティングを追記
> - 2026/05: 3D爆発エフェクト (Effect::kExplosion) 仕様とスニペットの追加
> - 2026/06: カスタムエディタUI構築時の Undo/Redo (Ctrl+Z) 対応用ヘルパー関数の解説を追記
> - 2026/06: プリミティブ描画の `Primitive3DObject` への統合、および `MeshDesc` / `MaterialDesc` へのデータ構造リファクタリングを反映
> - 2026/06: 描画リソースのマルチバッファ同期処理を `MultiBufferSyncState` 基底クラスへ集約し、`Manual.md` に使い方を追加
> - 2026/06: シェーダーのハイブリッドコンパイル（Releaseの完全CSO化）およびEditor/Debug向けのホットリロード機能の解説を追加
> - 2026/06: マルチスレッド化による GameObject::Update の並列処理と、ダングリングポインタ対策としての `std::weak_ptr` の利用ルールを追記
> - 2026/06: エディタ操作性向上（インスペクターのRemove/Reset機能、D&D割り当て）の解説を追記
> - 2026/06: Hierarchyでのオブジェクト管理機能（フォルダ化・ロック・保護機能）の解説を追加
> - 2026/06: メニューバーの刷新（モダンダークテーマ化）、Play/Pause機能による時間管理（DeltaTime/RealDeltaTime）、およびレイアウト初期化（Reset Layout）の解説を追加

---

## 9. マルチスレッド化とコンポーネント設計 (Multithreading & Components)

本エンジンの `BaseScene` では、パフォーマンス向上のため **すべての `GameObject` の `Update` および `Draw` が `ThreadPool` を用いて並列実行（マルチスレッド処理）** されます。
そのため、コンポーネントを設計・実装する際は、スレッドセーフ（競合が起きない安全なコード）を意識する必要があります。

### 9.1 他の GameObject への参照とダングリングポインタ対策
マルチスレッド環境下では、参照していた他の `GameObject`（例：敵がプレイヤーを追いかける際のプレイヤー情報）が、別のスレッドで同時に破壊（GCによってメモリ解放）される可能性があります。
**生ポインタ (`GameObject*`) をメンバ変数として長期間保持することは厳禁です（Use-After-Free の原因になります）。**

必ず `std::weak_ptr<GameObject>` を使用し、アクセスする瞬間だけ `lock()` を取得して生存確認を行ってください。

```cpp
// ❌ 悪い例（生ポインタ保持はクラッシュの原因）
// GameObject* targetObject_ = nullptr; 

// ⭕ 良い例（weak_ptr で保持）
std::weak_ptr<GameObject> targetObject_;

// --- 使い方 ---
// 1. 他のオブジェクトを設定する時 (shared_from_this() を渡す)
debrisComp->SetTarget(playerObj->shared_from_this());

// 2. 毎フレーム Update でアクセスする時
if (auto target = targetObject_.lock()) { // lock()で生存確認
    if (target->GetIsActive()) {
        auto transform = target->GetComponent<TransformComponent>();
        // targetの座標へ移動する処理など...
    }
} else {
    // ターゲットは既に破壊された場合の処理
}
```

### 9.2 コンポーネント実行中の構造変更 (AddChild / AddComponent) について
`GameObject::Update` は並列実行されていますが、「自分自身」の子リスト（`children_`）やコンポーネントリストを変更するような操作（例えば、Update 中に自身へ `AddComponent` したり `AddChild` すること）は、内部配列の再確保（Reallocation）を引き起こす可能性があるため、十分に注意してください。

- **新規オブジェクトの生成**: 新しいオブジェクトをシーンにスポーンさせる場合、`scene->AddGameObject(obj)` は内部でスレッドセーフなキュー(`pendingAdds_`)に積まれるため、Update 中に呼んでも安全です。
- **オブジェクトの破棄**: `gameObject_->Destroy()` も破棄フラグ (`isDestroyed_`) を立てるだけなので、Update 中に呼んでも安全です（次フレームの開始前に一括削除されます）。
