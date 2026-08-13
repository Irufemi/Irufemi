import bpy
import os

class MYADDON_OT_spawn_import_symbol(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_spawn_import_symbol"
    bl_label = "出現ポイントシンボルのImport"
    bl_description = "出現ポイントのシンボルをImportします"
    
    def execute(self, context):
        # 既にプロトタイプがインポートされているかチェック
        if "PlayerSpawn" in bpy.data.objects:
            self.report({'WARNING'}, "既にインポートされています")
            return {'CANCELLED'}

        # player/player.obj のパスを構築
        filepath = os.path.join(os.path.dirname(__file__), "player", "player.obj")
        if not os.path.exists(filepath):
            self.report({'ERROR'}, f"ファイルが見つかりません: {filepath}")
            return {'CANCELLED'}
        
        # モデルをインポート (設定を反映)
        bpy.ops.wm.obj_import('EXEC_DEFAULT', filepath=filepath, forward_axis='Z', up_axis='Y')
        
        # インポート直後はアクティブオブジェクトになるので、それを取得
        obj = bpy.context.active_object
        
        # トランスフォームを適用 (回転・スケール)
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
        
        # 名前とカスタムプロパティを設定
        obj.name = "PlayerSpawn"
        obj["type"] = "PlayerSpawn"
        
        # 現在のコレクション（シーン）から除外してプロトタイプ化（非表示にする）
        bpy.context.collection.objects.unlink(obj)
        
        self.report({'INFO'}, "シンボルをインポートしました")
        return {'FINISHED'}

class MYADDON_OT_spawn_create_symbol(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_spawn_create_symbol"
    bl_label = "出現ポイントシンボルの作成"
    bl_description = "出現ポイントのシンボルを作成します"
    
    def execute(self, context):
        print("出現ポイントのシンボルを作成します")
        
        # プロトタイプがまだない場合は自動でインポートする
        if "PlayerSpawn" not in bpy.data.objects:
            bpy.ops.myaddon.myaddon_ot_spawn_import_symbol()
            
        spawn_object = bpy.data.objects.get("PlayerSpawn")
        if not spawn_object:
            self.report({'ERROR'}, "シンボルのプロトタイプが見つかりません")
            return {'CANCELLED'}
            
        # 複数選択を防ぐために現在の選択を解除
        bpy.ops.object.select_all(action='DESELECT')
        
        # 複製
        new_obj = spawn_object.copy()
        
        # シーンにリンクして出現させる
        bpy.context.collection.objects.link(new_obj)
        
        # オブジェクト名を設定
        new_obj.name = "PlayerSpawn"
        
        # アクティブにして選択状態にする
        bpy.context.view_layer.objects.active = new_obj
        new_obj.select_set(True)
        
        return {'FINISHED'}
