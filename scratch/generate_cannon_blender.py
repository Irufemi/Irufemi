import bpy
import math
import os

def cleanup_existing():
    # 画面上の古いplayerCannon、Cube、Cylinderなどのオブジェクトを一掃する
    to_remove = []
    for obj in bpy.data.objects:
        name = obj.name.lower()
        if "playercannon" in name or "cube" in name or "cylinder" in name:
            to_remove.append(obj)
            
    for obj in to_remove:
        try:
            bpy.data.objects.remove(obj, do_unlink=True)
        except Exception:
            pass
            
    # 不要なメッシュデータをメモリから削除
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
    # 位置・回転・縮尺をメッシュデータに完全に適用して固定する
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def create_super_simple_cannon():
    cleanup_existing()
    
    # マテリアル設定（ミリタリーな金属質感2色のみ）
    mat_dark = get_or_create_material("Mtl_DarkMetal", (0.15, 0.15, 0.16, 1.0), metallic=0.8, roughness=0.4)
    mat_light = get_or_create_material("Mtl_LightMetal", (0.35, 0.36, 0.38, 1.0), metallic=0.8, roughness=0.3)
    
    parts = []
    
    # ----------------- 1. Cannon Base (大砲の四角い土台/本体) -----------------
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.4, 0.0), align='WORLD')
    base = bpy.context.active_object
    base.scale = (0.35, 0.45, 0.8) # 幅 0.35, 高さ 0.45, 奥行き 0.8
    base.data.materials.append(mat_dark)
    freeze_transform(base)
    parts.append(base)
    
    # ----------------- 2. Single Barrel (シンプルで太い1本砲身) -----------------
    # 円柱を作成し、90度回転させて前方（-Z方向）へ伸ばす
    bpy.ops.mesh.primitive_cylinder_add(radius=0.15, depth=2.0, vertices=16, location=(0.0, 0.4, -1.0), align='WORLD')
    barrel = bpy.context.active_object
    barrel.rotation_euler = (math.radians(90), 0, 0)
    barrel.data.materials.append(mat_light)
    freeze_transform(barrel)
    parts.append(barrel)
    
    # ----------------- 3. 結合 -----------------
    bpy.ops.object.select_all(action='DESELECT')
    for part in parts:
        part.select_set(True)
        
    bpy.context.view_layer.objects.active = base
    bpy.ops.object.join()
    
    base.name = "playerCannon"
    base.data.name = "playerCannonMesh"
    
    print("Success: Super Simple Cannon created!")
    return base

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
        
    print(f"Success: Exported Super Simple Cannon to {dest_path}")

if __name__ == "__main__":
    export_filepath = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player/playerCannon.obj"
    cannon_obj = create_super_simple_cannon()
    export_obj(cannon_obj, export_filepath)
