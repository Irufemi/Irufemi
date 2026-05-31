import bpy
import bmesh
import math
import os
from mathutils import Vector

# ============================================================
# ゼンマイキー (Wind-Up Key) - Blender Python Script
# 画像の「なんでもぜんまい」を完全再現
# ============================================================

def clear_scene():
    """シーンをクリアする"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for mesh in bpy.data.meshes:
        bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)


def create_solid_texture(name="zenmai_texture", color=(0.95, 0.65, 0.12, 1.0), size=1024):
    """べた塗り単色テクスチャ画像を作成"""
    img = bpy.data.images.new(name, width=size, height=size, alpha=True)
    # 全ピクセルを同じ色で塗る
    pixels = list(color) * (size * size)
    img.pixels[:] = pixels
    img.pack()
    return img


def create_golden_material(texture_img):
    """ゴールド/真鍮風マテリアルを作成（テクスチャベース）"""
    mat = bpy.data.materials.new(name="GoldenBrass")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    
    for node in nodes:
        nodes.remove(node)
    
    # Image Texture ノード
    tex_node = nodes.new('ShaderNodeTexImage')
    tex_node.location = (-400, 0)
    tex_node.image = texture_img
    
    # Principled BSDF
    bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Metallic'].default_value = 0.9
    bsdf.inputs['Roughness'].default_value = 0.18
    bsdf.inputs['Specular IOR Level'].default_value = 0.9
    
    # テクスチャ → Base Color
    links.new(tex_node.outputs['Color'], bsdf.inputs['Base Color'])
    
    # Output
    output = nodes.new('ShaderNodeOutputMaterial')
    output.location = (300, 0)
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])
    
    return mat


def create_shaft():
    """シャフト（軸部分）を作成 - 長い円柱"""
    shaft_radius = 0.12
    shaft_length = 1.4
    
    bpy.ops.mesh.primitive_cylinder_add(
        radius=shaft_radius,
        depth=shaft_length,
        location=(0, 0, shaft_length / 2),
        vertices=64
    )
    shaft = bpy.context.active_object
    shaft.name = "Shaft"
    
    # 滑らかなベベル
    bevel = shaft.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.01
    bevel.segments = 3
    bevel.limit_method = 'ANGLE'
    bevel.angle_limit = math.radians(60)
    bpy.ops.object.modifier_apply(modifier="Bevel")
    
    return shaft


def create_shaft_collar():
    """シャフトとハンドルの間の接続リング"""
    collar_radius = 0.2
    collar_height = 0.1
    collar_z = 1.15
    
    bpy.ops.mesh.primitive_cylinder_add(
        radius=collar_radius,
        depth=collar_height,
        location=(0, 0, collar_z),
        vertices=64
    )
    collar = bpy.context.active_object
    collar.name = "ShaftCollar"
    
    bevel = collar.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.015
    bevel.segments = 3
    bevel.limit_method = 'ANGLE'
    bevel.angle_limit = math.radians(60)
    bpy.ops.object.modifier_apply(modifier="Bevel")
    
    return collar


def create_square_tip():
    """シャフト先端の四角い差し込み部分"""
    tip_size = 0.09
    tip_length = 0.3
    
    bpy.ops.mesh.primitive_cube_add(
        size=1,
        location=(0, 0, -tip_length / 2)
    )
    tip = bpy.context.active_object
    tip.name = "SquareTip"
    tip.scale = (tip_size, tip_size, tip_length / 2)
    bpy.ops.object.transform_apply(scale=True)
    
    bevel = tip.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.006
    bevel.segments = 2
    bpy.ops.object.modifier_apply(modifier="Bevel")
    
    return tip


def create_butterfly_handle():
    """蝶型ハンドルを作成 - 画像の通りXZ平面に展開
    2つの大きな円形穴が左右対称に配置
    """
    
    handle_depth = 0.10          # Y方向の厚み（薄い板状）
    loop_outer_r = 0.55          # 外側ループ半径
    loop_inner_r = 0.40          # 内側穴半径
    loop_offset_x = 0.44         # ループ中心のX方向オフセット
    handle_z = 1.6               # ハンドル中心のZ位置
    
    # ========== 外形を構築 ==========
    
    # 右ループ（XZ平面に対して垂直な薄い円柱）
    bpy.ops.mesh.primitive_cylinder_add(
        radius=loop_outer_r,
        depth=handle_depth,
        location=(loop_offset_x, 0, handle_z),
        rotation=(math.pi / 2, 0, 0),  # Y軸方向に薄くする
        vertices=128
    )
    right_outer = bpy.context.active_object
    right_outer.name = "R_Outer"
    
    # 左ループ
    bpy.ops.mesh.primitive_cylinder_add(
        radius=loop_outer_r,
        depth=handle_depth,
        location=(-loop_offset_x, 0, handle_z),
        rotation=(math.pi / 2, 0, 0),
        vertices=128
    )
    left_outer = bpy.context.active_object
    left_outer.name = "L_Outer"
    
    # 中央接続ブリッジ（XZ平面上の四角形）
    bpy.ops.mesh.primitive_cube_add(
        size=1,
        location=(0, 0, handle_z)
    )
    center = bpy.context.active_object
    center.name = "Center"
    center.scale = (0.3, handle_depth / 2, loop_outer_r * 0.72)
    bpy.ops.object.transform_apply(scale=True)
    
    # 上部の接続をより滑らかにするための追加ブリッジ
    bpy.ops.mesh.primitive_cube_add(
        size=1,
        location=(0, 0, handle_z + loop_outer_r * 0.3)
    )
    upper_bridge = bpy.context.active_object
    upper_bridge.name = "UpperBridge"
    upper_bridge.scale = (0.2, handle_depth / 2, loop_outer_r * 0.3)
    bpy.ops.object.transform_apply(scale=True)
    
    # 下部のシャフト接続部
    bpy.ops.mesh.primitive_cylinder_add(
        radius=0.15,
        depth=0.2,
        location=(0, 0, handle_z - loop_outer_r * 0.72 - 0.05),
        vertices=64
    )
    lower_conn = bpy.context.active_object
    lower_conn.name = "LowerConn"
    
    # 全外形を結合
    bpy.ops.object.select_all(action='DESELECT')
    for obj_name in ["R_Outer", "L_Outer", "Center", "UpperBridge", "LowerConn"]:
        obj = bpy.data.objects.get(obj_name)
        if obj:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = right_outer
    bpy.ops.object.join()
    
    handle = bpy.context.active_object
    handle.name = "ButterflyHandle"
    
    # Remesh で滑らかに結合（高解像度）
    remesh = handle.modifiers.new(name="Remesh", type='REMESH')
    remesh.mode = 'VOXEL'
    remesh.voxel_size = 0.01  # 非常に細かいvoxelサイズで滑らか
    bpy.ops.object.modifier_apply(modifier="Remesh")
    
    # ========== 穴を開ける ==========
    
    # 右穴
    bpy.ops.mesh.primitive_cylinder_add(
        radius=loop_inner_r,
        depth=handle_depth * 5,
        location=(loop_offset_x, 0, handle_z),
        rotation=(math.pi / 2, 0, 0),
        vertices=128
    )
    right_hole = bpy.context.active_object
    right_hole.name = "R_Hole"
    
    # 左穴
    bpy.ops.mesh.primitive_cylinder_add(
        radius=loop_inner_r,
        depth=handle_depth * 5,
        location=(-loop_offset_x, 0, handle_z),
        rotation=(math.pi / 2, 0, 0),
        vertices=128
    )
    left_hole = bpy.context.active_object
    left_hole.name = "L_Hole"
    
    # Boolean Difference で穴を開ける
    bpy.context.view_layer.objects.active = handle
    handle.select_set(True)
    
    bool_r = handle.modifiers.new(name="BoolR", type='BOOLEAN')
    bool_r.operation = 'DIFFERENCE'
    bool_r.object = right_hole
    bool_r.solver = 'FAST'
    bpy.ops.object.modifier_apply(modifier="BoolR")
    
    bool_l = handle.modifiers.new(name="BoolL", type='BOOLEAN')
    bool_l.operation = 'DIFFERENCE'
    bool_l.object = left_hole
    bool_l.solver = 'FAST'
    bpy.ops.object.modifier_apply(modifier="BoolL")
    
    # 穴オブジェクトを削除
    bpy.ops.object.select_all(action='DESELECT')
    right_hole.select_set(True)
    left_hole.select_set(True)
    bpy.ops.object.delete()
    
    # エッジをベベルで丸める
    bevel = handle.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.008
    bevel.segments = 3
    bevel.limit_method = 'ANGLE'
    bevel.angle_limit = math.radians(35)
    bpy.ops.object.modifier_apply(modifier="Bevel")
    
    # Smooth shading
    bpy.ops.object.select_all(action='DESELECT')
    handle.select_set(True)
    bpy.context.view_layer.objects.active = handle
    bpy.ops.object.shade_smooth()
    
    return handle


def create_zenmai_key():
    """ゼンマイキー全体を組み立て"""
    
    clear_scene()
    # べた塗りテクスチャ作成
    tex_img = create_solid_texture(
        name="zenmai_texture",
        color=(0.95, 0.65, 0.12, 1.0),  # オレンジゴールド
        size=1024
    )
    gold_mat = create_golden_material(tex_img)
    
    # 各パーツ作成
    shaft = create_shaft()
    collar = create_shaft_collar()
    tip = create_square_tip()
    handle = create_butterfly_handle()
    
    # マテリアルを全パーツに適用
    for obj in [shaft, collar, tip, handle]:
        if obj.data.materials:
            obj.data.materials[0] = gold_mat
        else:
            obj.data.materials.append(gold_mat)
    
    # 全パーツを選択して結合
    bpy.ops.object.select_all(action='DESELECT')
    shaft.select_set(True)
    collar.select_set(True)
    tip.select_set(True)
    handle.select_set(True)
    bpy.context.view_layer.objects.active = handle
    bpy.ops.object.join()
    
    zenmai = bpy.context.active_object
    zenmai.name = "ZenmaiKey"
    
    bpy.ops.object.shade_smooth()
    bpy.ops.object.origin_set(type='ORIGIN_GEOMETRY', center='BOUNDS')
    
    return zenmai


def setup_scene():
    """シーン設定"""
    
    # ---- Key Light ----
    bpy.ops.object.light_add(
        type='AREA',
        location=(3, -3, 5),
        rotation=(math.radians(50), 0, math.radians(45))
    )
    key_light = bpy.context.active_object
    key_light.name = "KeyLight"
    key_light.data.energy = 400
    key_light.data.size = 5
    key_light.data.color = (1.0, 0.95, 0.85)
    
    # ---- Fill Light ----
    bpy.ops.object.light_add(
        type='AREA',
        location=(-4, 2, 3),
        rotation=(math.radians(60), 0, math.radians(-30))
    )
    fill_light = bpy.context.active_object
    fill_light.name = "FillLight"
    fill_light.data.energy = 180
    fill_light.data.size = 6
    fill_light.data.color = (0.85, 0.9, 1.0)
    
    # ---- Rim Light ----
    bpy.ops.object.light_add(
        type='SPOT',
        location=(0, 4, 4),
        rotation=(math.radians(-40), 0, math.radians(180))
    )
    rim_light = bpy.context.active_object
    rim_light.name = "RimLight"
    rim_light.data.energy = 300
    rim_light.data.spot_size = math.radians(60)
    rim_light.data.color = (1.0, 0.85, 0.6)
    
    # ---- Camera ----
    bpy.ops.object.camera_add(
        location=(4.5, -4.5, 3.0),
        rotation=(math.radians(55), 0, math.radians(45))
    )
    camera = bpy.context.active_object
    camera.name = "Camera"
    camera.data.lens = 60  # 広角気味にして全体を写す
    bpy.context.scene.camera = camera
    
    # カメラをゼンマイキーに向ける
    zenmai = bpy.data.objects.get("ZenmaiKey")
    if zenmai:
        constraint = camera.constraints.new(type='TRACK_TO')
        constraint.target = zenmai
        constraint.track_axis = 'TRACK_NEGATIVE_Z'
        constraint.up_axis = 'UP_Y'
    
    # ---- Render Settings (EEVEE) ----
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT'
    scene.eevee.taa_render_samples = 64
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    
    # ---- World ----
    world = bpy.context.scene.world
    if world is None:
        world = bpy.data.worlds.new("World")
        bpy.context.scene.world = world
    world.use_nodes = True
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs['Color'].default_value = (0.08, 0.08, 0.12, 1.0)
        bg.inputs['Strength'].default_value = 0.3



def get_output_dir():
    """出力ディレクトリを取得する（Blenderテキストエディタ対応）"""
    # 1. 既にblendファイルが保存されている場合、そのディレクトリを使う
    if bpy.data.filepath:
        return os.path.dirname(bpy.data.filepath)
    
    # 2. コマンドラインから実行時は __file__ が使える
    try:
        script_path = os.path.abspath(__file__)
        if os.path.exists(os.path.dirname(script_path)):
            return os.path.dirname(script_path)
    except NameError:
        pass
    
    # 3. フォールバック: デスクトップのTD3_1フォルダ
    fallback = os.path.join(os.path.expanduser("~"), "Desktop", "TD3_1")
    os.makedirs(fallback, exist_ok=True)
    return fallback


def main():
    """メイン実行関数"""
    print("=" * 50)
    print("ゼンマイキー生成開始...")
    print("=" * 50)
    
    zenmai = create_zenmai_key()
    setup_scene()
    
    # 出力ディレクトリを取得
    output_dir = get_output_dir()
    print(f"出力ディレクトリ: {output_dir}")
    
    # ---- UV展開 ----
    zenmai_obj = bpy.data.objects.get("ZenmaiKey")
    if zenmai_obj:
        bpy.ops.object.select_all(action='DESELECT')
        zenmai_obj.select_set(True)
        bpy.context.view_layer.objects.active = zenmai_obj
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
        bpy.ops.object.mode_set(mode='OBJECT')
        print("UV展開完了 (Smart UV Project)")
    
    # ---- テクスチャをファイルに保存 ----
    tex_img = bpy.data.images.get("zenmai_texture")
    if tex_img:
        tex_path = os.path.join(output_dir, "zenmai_texture.png")
        tex_img.filepath_raw = tex_path
        tex_img.file_format = 'PNG'
        tex_img.save()
        print(f"テクスチャ保存: {tex_path}")
    
    # ビューポート設定
    for area in bpy.context.screen.areas:
        if area.type == 'VIEW_3D':
            for space in area.spaces:
                if space.type == 'VIEW_3D':
                    space.shading.type = 'MATERIAL'
                    space.region_3d.view_perspective = 'CAMERA'
    
    # .blendファイルを保存
    blend_path = os.path.join(output_dir, "zenmai_key.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    print(f"Blendファイル保存: {blend_path}")
    
    # レンダリング出力
    render_path = os.path.join(output_dir, "zenmai_render.png")
    bpy.context.scene.render.filepath = render_path
    bpy.ops.render.render(write_still=True)
    print(f"レンダリング出力: {render_path}")
    
    print("=" * 50)
    print("ゼンマイキー生成完了！")
    print("=" * 50)


if __name__ == "__main__":
    main()
