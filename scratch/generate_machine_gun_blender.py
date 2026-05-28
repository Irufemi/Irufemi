import bpy
import math
import os

def cleanup_existing():
    # 画面上のゴミ（以前失敗したCubeやCylinder、playerMachineGunオブジェクト）を一掃する
    # これにより、古いバラバラのオブジェクトが残るのを完全に防ぎます。
    to_remove = []
    for obj in bpy.data.objects:
        name = obj.name.lower()
        if "playermachinegun" in name or "cube" in name or "cylinder" in name:
            to_remove.append(obj)
            
    for obj in to_remove:
        try:
            bpy.data.objects.remove(obj, do_unlink=True)
        except Exception:
            pass
            
    # 孤立した不要なメッシュデータをメモリから削除
    for mesh in bpy.data.meshes:
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)

def get_or_create_material(name, color, metallic=0.8, roughness=0.3):
    mat = bpy.data.materials.get(name)
    if mat:
        if mat.use_nodes:
            bsdf = mat.node_tree.nodes.get("Principled BSDF")
            if bsdf:
                bsdf.inputs['Base Color'].default_value = color
                bsdf.inputs['Metallic'].default_value = metallic
                bsdf.inputs['Roughness'].default_value = roughness
        return mat
        
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = color
        bsdf.inputs['Metallic'].default_value = metallic
        bsdf.inputs['Roughness'].default_value = roughness
    return mat

def freeze_transform(obj):
    # オブジェクトを選択し、アクティブにして、トランスフォーム（位置・回転・縮尺）を完全にフリーズ（適用）する。
    # これにより、メッシュデータがグローバル座標に焼き付き、結合したときに絶対にズレなくなります。
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def create_simple_heavy_machine_gun():
    cleanup_existing()
    
    # マテリアル設定（ミリタリーな質感に合う3色）
    mat_dark = get_or_create_material("Mtl_DarkMetal", (0.15, 0.15, 0.16, 1.0), metallic=0.9, roughness=0.4)
    mat_light = get_or_create_material("Mtl_LightMetal", (0.35, 0.36, 0.38, 1.0), metallic=0.8, roughness=0.3)
    mat_accent = get_or_create_material("Mtl_Accent", (0.22, 0.25, 0.15, 1.0), metallic=0.2, roughness=0.6) # オリーブドラブ（弾薬箱）
    
    parts = []
    
    # ----------------- 1. Base / Mount (接続台座) -----------------
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.2, 0.0), align='WORLD', rotation=(0, 0, 0))
    base = bpy.context.active_object
    base.scale = (0.2, 0.4, 0.4)
    base.data.materials.append(mat_dark)
    freeze_transform(base) # 生成直後に座標を完全に固定！
    parts.append(base)
    
    # ----------------- 2. Receiver (機関部本体) -----------------
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.6, 0.3), align='WORLD', rotation=(0, 0, 0))
    receiver = bpy.context.active_object
    receiver.scale = (0.32, 0.42, 1.2)
    receiver.data.materials.append(mat_dark)
    freeze_transform(receiver)
    parts.append(receiver)
    
    # 上部フィードカバー
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.82, 0.2), align='WORLD', rotation=(0, 0, 0))
    feed_cover = bpy.context.active_object
    feed_cover.scale = (0.24, 0.12, 0.9)
    feed_cover.data.materials.append(mat_dark)
    freeze_transform(feed_cover)
    parts.append(feed_cover)
    
    # ----------------- 3. Barrel Shroud (バレル根元の冷却ジャケット) -----------------
    bpy.ops.mesh.primitive_cylinder_add(radius=0.10, depth=0.6, vertices=12, location=(0.0, 0.6, -0.6), align='WORLD', rotation=(0, 0, 0))
    shroud = bpy.context.active_object
    shroud.rotation_euler = (math.radians(90), 0, 0)
    shroud.data.materials.append(mat_dark)
    freeze_transform(shroud) # 90度回転させた状態で座標を固定！
    parts.append(shroud)
    
    # ----------------- 4. Long Barrel (細く長い1本の銃身) -----------------
    bpy.ops.mesh.primitive_cylinder_add(radius=0.05, depth=1.8, vertices=8, location=(0.0, 0.6, -1.8), align='WORLD', rotation=(0, 0, 0))
    barrel = bpy.context.active_object
    barrel.rotation_euler = (math.radians(90), 0, 0)
    barrel.data.materials.append(mat_light)
    freeze_transform(barrel)
    parts.append(barrel)
    
    # 銃口の消炎器（マズル）
    bpy.ops.mesh.primitive_cylinder_add(radius=0.06, depth=0.15, vertices=8, location=(0.0, 0.6, -2.75), align='WORLD', rotation=(0, 0, 0))
    muzzle = bpy.context.active_object
    muzzle.rotation_euler = (math.radians(90), 0, 0)
    muzzle.data.materials.append(mat_dark)
    freeze_transform(muzzle)
    parts.append(muzzle)
    
    # ----------------- 5. Ammo Box (側面の弾薬箱) -----------------
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(-0.35, 0.5, 0.4), align='WORLD', rotation=(0, 0, 0))
    ammo_box = bpy.context.active_object
    ammo_box.scale = (0.3, 0.45, 0.6)
    ammo_box.data.materials.append(mat_accent)
    freeze_transform(ammo_box)
    parts.append(ammo_box)
    
    # ----------------- 6. Butterfly Trigger Grip (後部のダブルハンドル) -----------------
    # 横バー
    bpy.ops.mesh.primitive_cylinder_add(radius=0.03, depth=0.45, vertices=8, location=(0.0, 0.6, 0.92), align='WORLD', rotation=(0, 0, 0))
    grip_bar = bpy.context.active_object
    grip_bar.rotation_euler = (0, math.radians(90), 0)
    grip_bar.data.materials.append(mat_dark)
    freeze_transform(grip_bar)
    parts.append(grip_bar)
    
    # 左縦グリップ
    bpy.ops.mesh.primitive_cylinder_add(radius=0.03, depth=0.35, vertices=8, location=(-0.2, 0.6, 0.98), align='WORLD', rotation=(0, 0, 0))
    grip_l = bpy.context.active_object
    grip_l.rotation_euler = (math.radians(90), 0, 0)
    grip_l.data.materials.append(mat_light)
    freeze_transform(grip_l)
    parts.append(grip_l)
    
    # 右縦グリップ
    bpy.ops.mesh.primitive_cylinder_add(radius=0.03, depth=0.35, vertices=8, location=(0.2, 0.6, 0.98), align='WORLD', rotation=(0, 0, 0))
    grip_r = bpy.context.active_object
    grip_r.rotation_euler = (math.radians(90), 0, 0)
    grip_r.data.materials.append(mat_light)
    freeze_transform(grip_r)
    parts.append(grip_r)

    # ----------------- 7. Join All Parts (オブジェクトの統合) -----------------
    # すべてのパーツを選択
    bpy.ops.object.select_all(action='DESELECT')
    for part in parts:
        part.select_set(True)
        
    bpy.context.view_layer.objects.active = receiver
    bpy.ops.object.join()
    
    # 統合したオブジェクトの名前を設定
    receiver.name = "playerMachineGun"
    receiver.data.name = "playerMachineGunMesh"
    
    print("Success: Simple Heavy Machine Gun model created inside Blender!")
    return receiver

def export_obj(obj, dest_path):
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    
    try:
        # Blender 4.0+
        bpy.ops.wm.obj_export(
            filepath=dest_path,
            export_selected_objects=True,
            export_materials=True,
            export_colors=True,
            export_normals=True,
            export_uv=True,
            forward_axis='NEGATIVE_Z',
            up_axis='Y'
        )
    except AttributeError:
        # 旧バージョン用
        bpy.ops.export_scene.obj(
            filepath=dest_path,
            use_selection=True,
            use_materials=True,
            axis_forward='-Z',
            axis_up='Y'
        )
        
    print(f"Success: Exported 3D model to {dest_path}")

if __name__ == "__main__":
    # 出力パスの設定
    export_filepath = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player/playerMachineGun.obj"
    
    gun_obj = create_simple_heavy_machine_gun()
    export_obj(gun_obj, export_filepath)
