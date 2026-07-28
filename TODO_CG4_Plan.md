# CG4 評価課題2 実装計画・検討用ドキュメント

出先での作業用として、実装候補とその詳細を詰めるためのドキュメントです。
何が実装済みで、何が未実装か、またどのようなアプローチで実装するかをここで議論・記録していきます。

## 加点要素・実装候補リスト

ステータス（`[ ]` 未着手, `[x]` 完了/自宅データにあり, `[?]` 相談中/出先で作業）をマークして管理します。

- [x] **Skinningモデルの表示しパッドで動かせる (20点)** 【事実上の必須要件】
  - 現状: エンジン側に Compute Shader スキニング機能は実装済み。
  - 詳細・実装方針: アプリ層にて評価課題用の `CG4Scene` を新規実装する。外部ライブラリのEditorシーン管理を利用し、`CG4Scene` 内でパッド入力を受け取って `AnimatedMeshObject` の移動・制御処理を構築する。
- [x] **ComputeShaderによるスキニング (10点)**
  - 現状: `DrawManager::DispatchSkinning` および `Skinning.CS.hlsl` にて既に実装完了済み。
- [x] **MultiMesh & MultiMaterial対応 (5点)** 【事実上の必須要件】
  - 現状: `ObjMesh` に `nodeName` は追加されているが、非スキニング時に各ノードの階層ごとのローカル行列が考慮されていない（Rootの行列だけ掛かっている）。
  - 詳細・実装方針:
    1. `ObjModel` のパース時（`ModelLoader`）に、各 Node の階層情報をフラットな配列またはインデックスアクセスできる形（`std::vector<Matrix4x4> globalTransforms`）で持てるようにする。
    2. `StaticModelObject::SyncBeforeDraw` 等で、ルートから再帰的に Node を巡回し、`globalMatrix = parentGlobal * localMatrix` を計算してキャッシュ配列に格納する。
    3. `StaticModelObject::Draw` 内のメッシュループにおいて、対象の `ObjMesh` が属するノードの `globalMatrix` を取得し、個別の Transform 定数バッファ（あるいは構造化バッファのインデックス）を更新して `SubmitStandard3D` へ渡す。

- [x] **Animation補間 (5点)**
  - 現状: `AnimationManager::BlendAnimation` にて線形補間・球面線形補間を用いたブレンド処理が実装済み。`Animator` クラスでのフェードタイマー制御（`isBlending_` 等）による遷移も実装済み。
  - 詳細・実装方針:
    1. `Animator::Play` を拡張（または `CrossFade` 関数を追加）し、再生中のアニメーションを `previous` に退避し、`fadeTimer_ = 0` にリセットする。
    2. `Animator::Update` 内で、`isBlending_` が true ならば `fadeTimer_` を進め、`weight = fadeTimer_ / fadeDuration_` を算出して `BlendAnimation` を呼び出す。
    3. `weight >= 1.0f` になったら `isBlending_ = false` とし、単一アニメーションの再生（`ApplyAnimation`）に移行する。
    - **※確認結果: Animator.cpp にて既に実装完了済みであることを確認しました！**

- [ ] **手からパーティクルを出す (10点)**
  - 現状: `SkeletonPose` には各ボーンの `skeletonSpaceMatrix` があるが、外部から特定のボーンのワールド行列を取得する口がない。
  - 詳細・実装方針 (Editor拡張含む):
    1. `Animator` 等に `Matrix4x4 GetJointWorldMatrix(const std::string& jointName)` を追加。
    2. アプリ層（ECS側）に `SocketComponent` を作成。プロパティとして「対象のEntity」「追従するボーン名(`jointName`)」「オフセットTransform（位置・回転）」を持たせる。
    3. **【Editor連携】**: `SocketComponent` のインスペクタUI（`OnRegisterProperties`）で、`ImGui::Combo` による文字化け・クラッシュを防ぐため、**安全な `ImGui::BeginCombo` と `ImGui::Selectable`** を用いて `SkeletonData::joints` の一覧をプルダウン表示する。
    4. さらにオフセット値をEditor上でリアルタイムにスライダー調整できるようにし、調整した結果をエミッタの放出原点として適用する。

- [ ] **武器を手に持たせる (10点)**
  - 現状: 上記のパーティクルと同じく、ボーンのワールド座標を取得する仕組みが必要。
  - 詳細・実装方針 (Editor拡張含む):
    1. 「手からパーティクルを出す」で強化した `SocketComponent` を共用する。
    2. ソケットの子Entityに武器のモデル（`MeshRenderer`）を持たせる。
    3. **【Editor連携】**: `BeginCombo` を用いたボーン選択UIからアタッチ先の関節を選びつつ、武器の「握り位置（オフセット）」や「角度」をスライダーで微調整して自然に持たせる。

- [ ] **骨のデバッグ表示 (10点)**
  - 現状: ボーンの階層構造（親子関係）は `SkeletonData` にあり、現在のポーズは `SkeletonPose` にある。
  - 詳細・実装方針:
    1. `SkinnedMeshRendererComponent::Draw` の後、または専用の `SkeletonDebugRendererComponent` を作成する。
    2. `SkeletonData::joints` をループし、`parent` が存在するなら、親のジョイントのワールド座標と自身のワールド座標を `PrimitiveRendererComponent`（または `DrawManager` のライン描画API）に渡して線分を描画する。
    3. ジョイントの位置に XYZ 軸を示す短い3色のライン（赤・緑・青）を描画してローカル軸を可視化する。

- [x] **GPU Particle (20〜30点)**
  - 実装完了:
    1. Meshエミッタを実装し、ランダムな頂点から法線方向にパーティクルを射出するCSを追加。
    2. Trail（軌跡）やDeath Emit（消滅時破裂）の拡張パラメータ対応。
    3. 重力・風・Vortexなどのフィールド（Field）影響機能を実装（`ParticleFieldComponent`）。
    4. Editorから `emitType=6` (Mesh) 等を選択・操作できるように `GPUParticleManager`、`GPUParticleSystem` 等を対応。

- [ ] **その他 (10点)**
  - エンジン開発計画にある高度な機能を実装して加点を狙う。本課題のイチオシは以下の「視覚的ボーンマスクエディタ」の実装。
  
  - **★メイン候補: 視覚的ボーンマスクエディタ (Bone Masking + Skeleton Tree View の合体技)**
    - *概要*: エディタのインスペクタ上でスケルトンの階層構造をツリー表示し、チェックボックスで視覚的に「部分ブレンド」の境界を決定できる、商用エンジン（Unity/UE）同等の強力なアニメーション制御機能。
    - *詳細・実装方針*:
      1. **Editor UI (Tree View)**: `SkinnedMeshRendererComponent::OnRegisterProperties` にて、再帰関数と `ImGui::TreeNode`、`ImGui::Checkbox` を用いてボーン階層を描画する。
      2. **連動チェック機能**: ツリー上で特定のボーン（例: Spine）にチェックを入れると、そのボーンの子孫すべてのチェック状態が自動で `true` になるロジックを組む。
      3. **Bone Mask 配列**: チェックボックスのON/OFF状態を `std::vector<bool> boneMask_` として保持する。
      4. **部分ブレンド再生**: `Animator` に2つのアニメーション（BaseとUpperBody）を持たせ、`SkeletonPose` 更新時に、`boneMask_[i] == true` なら UpperBody、`false` なら Base の Transform を適用する。
      5. **プログラム制御 (動的生成)**: エディタを使わずにコード側から境界ボーン（例："Spine"）を指定した場合に備え、特定のジョイントの子孫かを親に遡って判定する関数 `bool IsDescendant(int32_t jointIndex, int32_t ancestorIndex)` を用いたマスク配列の自動生成ルートも備えておく。

  - **候補2: IK (Inverse Kinematics)**
    - *概要*: Look-At IK（敵の方向に常に顔を向ける）などのプロシージャルな骨格制御。
    - *詳細・実装方針*:
      1. `Animator::Update` で全ボーンの `skeletonSpaceMatrix` が計算された**後**に処理を挟む。
      2. 「首(Neck)」ジョイントの現在位置から、注視対象（Target）への方向ベクトルを算出。
      3. 現在の首の向きと目標方向の差分回転（`FromToRotation`等）を求め、首のローカル Quaternion に乗算。
      4. 首から下（子孫）の `skeletonSpaceMatrix` を再計算して姿勢を上書きする。

  - **候補3: オニオンスキニング (軌跡のデバッグ表示)**
    - *概要*: 過去フレームの姿勢残像によるモーション軌道の可視化。
    - *詳細・実装方針*: `Animator` 内に `std::deque<std::vector<JointPose>> poseHistory_` (リングバッファ) を持たせ、デバッグ描画時に古い履歴ほど透明度を下げて半透明ラインで同時描画する。

  - **候補4: 加算ブレンド (Additive Blending) ＋ 破綻回避（ウェイト）制御**
    - *概要*: 基本のモーションに対し、ダメージの「のけぞり差分」だけを上乗せして再生する機構。関節が不自然に折れ曲がる破綻を防ぐための安全装置も組み込む。
    - *詳細・実装方針*: 
      1. ベースポーズに対する「差分 Transform」を事前計算してデータ化する。
      2. `Animator::PlayAdditive(weight)` のように「ブレンド率（強さ）」を引数やメンバ変数として持たせる。
      3. 毎フレームの合成時、ベースの `JointPose` に対して差分をそのまま足すのではなく、`weight` (0.0〜1.0) で減衰（QuaternionのSlerp等を使用）させてから合成し、関節がバキッと折れるのを防ぐ。
## 🚨 必須リファクタリング (潜在バグの修正)
今回の加点要素（アクション遷移等）を安全に実装するため、既存のエンジンコードに潜む以下の致命的な不具合を事前に修正する。

- [x] **Root Motionの二重適用（超高速回転バグ）の修正**
  - *対象ファイル*: `AnimationManager.cpp` (`BlendAnimation` / `ApplyAnimation`)
  - *問題*: `!applyRootTranslation` の分岐で「移動」はスキップされているが、「回転(rotate)」と「スケール(scale)」がスキップされておらず、GameObjectとボーンの両方に回転が二重適用されてしまう。
  - *修正方針*: `isRoot` かつルート適用を除外する場合、`rotate` と `scale` もLerp/Slerp処理をスキップ（または初期値を維持）するように `if-else` のスコープを修正する。

- [x] **Root抽出アルゴリズムの脆弱性修正**
  - *対象ファイル*: `Animator.cpp` (`ExtractRootMotion`)
  - *問題*: 現在「一番最初のノード」をルートとして抽出しているため、CameraやWeaponノードが先頭に来るFBXを読むと座標が吹き飛ぶ。
  - *修正方針*: `SkeletonData::joints` を参照し、`parent` が存在しない本物のルートノード（"Root" やインデックス0など確実なもの）の `nodeName` と一致するアニメーションノードから差分を抽出するようにロジックを改修する。

## 議論メモ
- 出先環境（今のデータ状態）でも進めやすい・設計しやすい項目はどれか？
