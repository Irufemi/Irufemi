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
class ObjClass;

class ExampleScene : public BaseScene {
public:
    ~ExampleScene() override = default;
    
    // シーン遷移時に一度だけ呼ばれる（リソース読み込みなど）
    void Initialize(IrufemiEngine* engine) override;
    
    // 毎フレーム呼ばれる（ロジックの更新）
    void Update() override;
    
    // 毎フレーム呼ばれる（描画リクエストの送信）
    void Draw() override;

private:
    std::unique_ptr<ObjClass> playerObj_;
};
```

#### シーン遷移の実行方法
シーンを移動するには `SceneManager::TransitionTo` を呼び出します。
```cpp
// Update関数の中などで条件を満たした場合
if (isClear) {
    // "ClearScene" に遷移する。トランジション効果は Fade で 1.0秒かける
    engine_->GetSceneManager()->TransitionTo("ClearScene", SceneTransition::Type::Fade, 1.0f);
}
```

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

// ※デバッグカメラへの切り替え
cameraManager->SetActiveCamera("DebugCamera");
// 元のカメラに戻す
cameraManager->SetActiveCamera("MainCamera");
```

---

## 2. 描画・オブジェクトシステム (Rendering System)

画面にモノを表示するための主要なクラス群です。

### 2.1 3Dモデル描画 (`ObjClass` / `AnimationModel`)
静的な3Dモデルを表示するには `ObjClass` を使います。モデルデータは `ModelManager` を経由して自動的にキャッシュされます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<ObjClass> model_;

// 2. 初期化 (Initialize)
model_ = std::make_unique<ObjClass>();
model_->Initialize("enemy/enemy.obj"); // resources/model/ 以下のパスを指定
model_->SetPosition({ 0.0f, 0.0f, 10.0f });
model_->SetScale({ 2.0f, 2.0f, 2.0f });

// 3. 更新 (Update)
model_->Update(); // ※毎フレーム必ず呼ぶこと（ワールド行列が更新されます）

// 4. 描画 (Draw)
model_->Draw();   // DrawManagerの標準3D描画キューに登録される
```

### 2.2 2Dスプライト描画 (`Sprite`)
画面にUIなどの2D画像を表示するためのクラスです。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<Sprite> sprite_;

// 2. 初期化 (Initialize)
sprite_ = std::make_unique<Sprite>();
sprite_->Initialize("ui/title_logo.png"); // resources/ 以下のパスを指定
sprite_->SetPosition({ 640.0f, 360.0f }); // 画面中央 (1280x720の場合)
sprite_->SetAnchorPoint({ 0.5f, 0.5f });  // 画像の中心を基準にする

// 3. 更新 (Update)
sprite_->Update();

// 4. 描画 (Draw)
sprite_->Draw(); // 通常のUIパスで描画される

// ※ポストプロセス（ブルーム等）の影響を受けない最前面UIとして描画したい場合
// sprite_->Draw(true); 
```

### 2.3 プリミティブ形状 (`CubeClass`, `SphereClass`, `CylinderClass`, `LineClass`)
当たり判定のデバッグ表示や、プロトタイプの作成に便利な組み込み図形です。モデルファイルなしで使えます。

```cpp
// 1. 宣言 (ヘッダー)
std::unique_ptr<CubeClass> cube_;
std::unique_ptr<LineClass> line_;

// 2. 初期化 (Initialize)
cube_ = std::make_unique<CubeClass>();
cube_->Initialize();
cube_->SetColor({ 1.0f, 0.0f, 0.0f, 0.5f }); // 赤色で半透明

line_ = std::make_unique<LineClass>();
line_->Initialize();
line_->SetColor({ 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色の線

// 3. 更新と描画 (Update & Draw)
cube_->SetPosition(playerPos);
cube_->Update();
cube_->Draw();

line_->SetStartAndEnd(startPos, endPos);
line_->Update();
line_->Draw(); // ライン専用のキューに登録される
```

### 2.4 パーティクル (`GPUParticleSystem`)
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

---

## 4. 入力・演算・ユーティリティ (Input & Utility)

プレイヤーの操作を受け取ったり、ゲームに必要な計算を行うクラス群です。

### 4.1 InputManager (入力の取得)
キーボード、マウス、ゲームパッド（XInput互換）の操作をまとめて取得できます。
（`Scene` の `Update()` 内で `engine_->GetInputManager()` を介してアクセスします）

```cpp
auto input = engine_->GetInputManager();

// 【キーボード】
// Spaceキーが「押された瞬間」か？
if (input->IsKeyPressed(DIK_SPACE)) { ... }
// Wキーが「押され続けている」か？
if (input->IsKeyDown(DIK_W)) { ... }

// 【ゲームパッド】
// Aボタンが押された瞬間か？
if (input->IsButtonPressed(XINPUT_GAMEPAD_A)) { ... }
// 左スティックの入力値 (-1.0f 〜 1.0f)
float lx = input->GetLeftStickX();
float ly = input->GetLeftStickY();

// 【マウス】
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
> **ドキュメント更新履歴**
> - 2026/05: RenderGraph / パケット分離対応を反映、Phase 1~3 および 便利機能（カメラ、ディレクトリ構造、UISelectionGroup）を追記
