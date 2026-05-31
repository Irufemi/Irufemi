import bpy
import math
import os

def cleanup_existing():
    to_remove = []
    for obj in bpy.data.objects:
        name = obj.name.lower()
        if any(x in name for x in ["windup", "key", "shaft", "magnet", "bridge", "neck", "collar", "cylinder", "cube"]):
            to_remove.append(obj)
    for obj in to_remove:
        try:
            bpy.data.objects.remove(obj, do_unlink=True)
        except Exception:
            pass
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
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs['Base Color'].default_value = color
        bsdf.inputs['Metallic'].default_value = metallic
        bsdf.inputs['Roughness'].default_value = roughness
    return mat

def freeze(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def create_toy_windup_key():
    cleanup_existing()

    mat_chrome = get_or_create_material("Mtl_WindupChrome", (0.75, 0.75, 0.76, 1.0), metallic=1.0, roughness=0.15)
    mat_magnet = get_or_create_material("Mtl_WindupMagnet", (0.08, 0.08, 0.09, 1.0), metallic=0.1, roughness=0.45)

    parts = []

    # --- Blender座標系: Z=上, X=左右, Y=奥行き ---

    # 1. マグネット台座 (Z=0付近)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0))
    mag = bpy.context.active_object
    mag.scale = (0.12, 0.09, 0.04)
    freeze(mag)
    mag.name = "Windup_MagnetBase"
    mag.data.materials.append(mat_magnet)
    parts.append(mag)

    # 2. カラー (シャフト根元リング)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.095, depth=0.025, vertices=16, location=(0, 0, 0.0525))
    collar = bpy.context.active_object
    freeze(collar)
    collar.name = "Windup_Collar"
    collar.data.materials.append(mat_chrome)
    parts.append(collar)

    # 3. シャフト (Z軸方向に上へ伸びる)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.08, depth=0.54, vertices=24, location=(0, 0, 0.335))
    shaft = bpy.context.active_object
    freeze(shaft)
    shaft.name = "Windup_Shaft"
    shaft.data.materials.append(mat_chrome)
    parts.append(shaft)

    # 4. ネック (シャフト上端とブリッジの接続部)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.59))
    neck_block = bpy.context.active_object
    neck_block.scale = (0.10, 0.04, 0.11)
    freeze(neck_block)
    neck_block.name = "Windup_Neck"
    neck_block.data.materials.append(mat_chrome)
    parts.append(neck_block)

    # 5. ブリッジ (左右の耳をつなぐ水平プレート)
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0, 0, 0.78))
    bridge = bpy.context.active_object
    bridge.scale = (0.40, 0.04, 0.18)
    freeze(bridge)
    bridge.name = "Windup_Bridge"
    bridge.data.materials.append(mat_chrome)
    parts.append(bridge)

    # 6. 左耳 (外円 - 穴あけ用Boolean)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=0.08, vertices=32, location=(-0.40, 0, 0.78))
    left_ear = bpy.context.active_object
    left_ear.rotation_euler = (math.radians(90), 0, 0)
    freeze(left_ear)
    left_ear.name = "Windup_LeftEar"

    # 7. 右耳 (外円)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.34, depth=0.08, vertices=32, location=(0.40, 0, 0.78))
    right_ear = bpy.context.active_object
    right_ear.rotation_euler = (math.radians(90), 0, 0)
    freeze(right_ear)
    right_ear.name = "Windup_RightEar"

    # ブリッジと左右耳を結合
    bpy.ops.object.select_all(action='DESELECT')
    bridge.select_set(True)
    left_ear.select_set(True)
    right_ear.select_set(True)
    bpy.context.view_layer.objects.active = bridge
    bpy.ops.object.join()
    key_head = bridge

    # 8. 左穴 (Boolean差分)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.19, depth=0.2, vertices=32, location=(-0.40, 0, 0.78))
    left_hole = bpy.context.active_object
    left_hole.rotation_euler = (math.radians(90), 0, 0)
    freeze(left_hole)

    bpy.context.view_layer.objects.active = key_head
    bl = key_head.modifiers.new(name="LH", type='BOOLEAN')
    bl.operation = 'DIFFERENCE'
    bl.object = left_hole
    bpy.ops.object.modifier_apply(modifier="LH")

    # 9. 右穴 (Boolean差分)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.19, depth=0.2, vertices=32, location=(0.40, 0, 0.78))
    right_hole = bpy.context.active_object
    right_hole.rotation_euler = (math.radians(90), 0, 0)
    freeze(right_hole)

    bpy.context.view_layer.objects.active = key_head
    br = key_head.modifiers.new(name="RH", type='BOOLEAN')
    br.operation = 'DIFFERENCE'
    br.object = right_hole
    bpy.ops.object.modifier_apply(modifier="RH")

    bpy.data.objects.remove(left_hole, do_unlink=True)
    bpy.data.objects.remove(right_hole, do_unlink=True)

    # 10. ベベル (角の丸め)
    bpy.context.view_layer.objects.active = key_head
    bv = key_head.modifiers.new(name="Bevel", type='BEVEL')
    bv.width = 0.015
    bv.segments = 3
    bpy.ops.object.modifier_apply(modifier="Bevel")

    key_head.name = "Windup_KeyHead"
    key_head.data.materials.append(mat_chrome)
    parts.append(key_head)

    # 11. 全結合
    bpy.ops.object.select_all(action='DESELECT')
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()

    result = parts[0]
    result.name = "windup_mechanism"
    result.data.name = "windup_mechanism_mesh"
    print("Success: Toy Wind-up Key with BOTH ears created!")
    return result

def export_obj(obj, dest_path):
    os.makedirs(os.path.dirname(dest_path), exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    try:
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
        bpy.ops.export_scene.obj(
            filepath=dest_path,
            use_selection=True,
            use_materials=True,
            axis_forward='-Z',
            axis_up='Y'
        )
    print(f"Success: Exported to {dest_path}")

if __name__ == "__main__":
    export_filepath = "c:/Users/k024g/Desktop/TD3_1/project/Application/resources/model/player/windup_mechanism.obj"
    obj = create_toy_windup_key()
    export_obj(obj, export_filepath)
