import bpy
import math
import os

def create_rocket_launcher():
    # 既存のメッシュオブジェクトを削除してクリーンアップ
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.object.select_by_type(type='MESH')
    bpy.ops.object.delete()

    # --- 1. マテリアルの作成と設定 ---
    # ネイビーブルー (DarkMetal)
    mat_dark = bpy.data.materials.new(name="Mtl_DarkMetal")
    mat_dark.use_nodes = True
    nodes_dark = mat_dark.node_tree.nodes
    nodes_dark.clear()
    
    # BSDFとマテリアル出力(Material Output)ノードを作成して接続
    bsdf_dark = nodes_dark.new(type='ShaderNodeBsdfPrincipled')
    output_dark = nodes_dark.new(type='ShaderNodeOutputMaterial')
    mat_dark.node_tree.links.new(bsdf_dark.outputs["BSDF"], output_dark.inputs["Surface"])
    
    # Blender 4.0+ / 3.x 両対応でBase Colorを設定
    if hasattr(bsdf_dark.inputs.get("Base Color"), "default_value"):
        bsdf_dark.inputs["Base Color"].default_value = (0.05, 0.12, 0.36, 1.0)
    if hasattr(bsdf_dark.inputs.get("Roughness"), "default_value"):
        bsdf_dark.inputs["Roughness"].default_value = 0.4

    # シアンブルー (LightMetal)
    mat_light = bpy.data.materials.new(name="Mtl_LightMetal")
    mat_light.use_nodes = True
    nodes_light = mat_light.node_tree.nodes
    nodes_light.clear()
    
    # BSDFとマテリアル出力(Material Output)ノードを作成して接続
    bsdf_light = nodes_light.new(type='ShaderNodeBsdfPrincipled')
    output_light = nodes_light.new(type='ShaderNodeOutputMaterial')
    mat_light.node_tree.links.new(bsdf_light.outputs["BSDF"], output_light.inputs["Surface"])
    
    if hasattr(bsdf_light.inputs.get("Base Color"), "default_value"):
        bsdf_light.inputs["Base Color"].default_value = (0.0, 0.75, 1.0, 1.0)
    if hasattr(bsdf_light.inputs.get("Roughness"), "default_value"):
        bsdf_light.inputs["Roughness"].default_value = 0.3

    # --- 2. ポッドケース本体 (直方体) の作成 ---
    BW, BH, BD = 1.0, 0.85, 1.5
    bpy.ops.mesh.primitive_cube_add(
        size=1.0,
        calc_uvs=True,
        enter_editmode=False,
        align='WORLD',
        location=(0, 0, 0)
    )
    pod_obj = bpy.context.active_object
    pod_obj.name = "PodCase"
    # スケールを適用して所望の直方体サイズにする (Cube size=1.0 なので X=2*BW, Y=2*BH, Z=2*BD)
    pod_obj.scale = (BW * 2, BH * 2, BD * 2)
    bpy.ops.object.transform_apply(scale=True)
    
    # マテリアル設定 (ネイビー)
    pod_obj.data.materials.append(mat_dark)

    # --- 3. 4本の発射管チューブ (円柱) の作成 ---
    R = 0.28
    L = 3.0  # 長さ
    # Zの範囲が 1.2 から -1.8 なので、中心は -0.3
    z_center = -0.3
    
    offsetX = 0.45
    offsetY = 0.38
    
    tube_positions = [
        (offsetX, offsetY, z_center),
        (-offsetX, offsetY, z_center),
        (offsetX, -offsetY, z_center),
        (-offsetX, -offsetY, z_center)
    ]
    
    tubes = []
    for i, pos in enumerate(tube_positions):
        bpy.ops.mesh.primitive_cylinder_add(
            vertices=12,
            radius=R,
            depth=L,
            calc_uvs=True,
            enter_editmode=False,
            align='WORLD',
            location=pos
        )
        tube_obj = bpy.context.active_object
        tube_obj.name = f"LauncherTube_{i+1}"
        
        # マテリアル設定 (シアン)
        tube_obj.data.materials.append(mat_light)
        tubes.append(tube_obj)

    # --- 4. オブジェクトの結合と仕上げ ---
    # ポッド本体とチューブを全て選択
    bpy.ops.object.select_all(action='DESELECT')
    pod_obj.select_set(True)
    for tube in tubes:
        tube.select_set(True)
        
    # ポッド本体をアクティブにして結合 (Join)
    bpy.context.view_layer.objects.active = pod_obj
    bpy.ops.object.join()
    
    launcher_obj = bpy.context.active_object
    launcher_obj.name = "playerRocketLauncher"

    # スマートUV投影でUV展開をやり直す (綺麗なテクスチャマッピングのため)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66.0, island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')

    # エクスポートパス設定
    export_dir = r"c:\Users\k024g\Desktop\TD3_1\project\Application\resources\model\player"
    if not os.path.exists(export_dir):
        os.makedirs(export_dir)
        
    obj_path = os.path.join(export_dir, "playerRocketLauncher.obj")

    # OBJエクスポート
    try:
        if hasattr(bpy.ops.export_scene, "obj"):
            bpy.ops.export_scene.obj(
                filepath=obj_path,
                use_selection=True,
                use_materials=True,
                keep_vertex_order=True
            )
        else:
            bpy.ops.wm.obj_export(
                filepath=obj_path,
                export_selected_objects=True
            )
        print(f"Successfully exported launcher model to: {obj_path}")
    except Exception as e:
        print(f"Export failed: {e}")

if __name__ == "__main__":
    create_rocket_launcher()
