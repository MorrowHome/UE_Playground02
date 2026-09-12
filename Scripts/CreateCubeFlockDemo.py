"""Run with UnrealEditor-Cmd -run=pythonscript -script=<absolute path>.
Creates only /Game/CubeFlockDemo assets. Re-running rebuilds the demo map/material.
"""
import unreal

ROOT = "/Game/CubeFlockDemo"
MAP = ROOT + "/Lvl_CubeFlock"
MAT_PATH = ROOT + "/M_GPUCubeFlock"
lib = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets.make_directory(ROOT)

material = assets.load_asset(MAT_PATH) if assets.does_asset_exist(MAT_PATH) else None
if material is None:
    material = tools.create_asset("M_GPUCubeFlock", ROOT, unreal.Material, unreal.MaterialFactoryNew())
else:
    lib.delete_all_material_expressions(material)
material.set_editor_property("used_with_instanced_static_meshes", True)
material.set_editor_property("max_world_position_offset_displacement", 5000.0)

def node(cls, x, y):
    return lib.create_material_expression(material, cls, x, y)

custom = node(unreal.MaterialExpressionCustom, -150, 0)
custom.set_editor_property("description", "GPU position minus instance rest position")
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
names = ["Id", "RestX", "RestY", "RestZ", "Positions", "Enabled"]
inputs = []
for name in names:
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
custom.set_editor_property("inputs", inputs)
custom.set_editor_property("code", """
float2 UV = (float2(fmod(Id, 64.0), floor(Id / 64.0)) + 0.5) / 64.0;
float3 SimPosition = Texture2DSampleLevel(Positions, PositionsSampler, UV, 0).xyz;
return (SimPosition - float3(RestX, RestY, RestZ)) * Enabled;
""")
for i, name in enumerate(names[:4]):
    data = node(unreal.MaterialExpressionPerInstanceCustomData, -650, i * 100)
    data.set_editor_property("data_index", i)
    lib.connect_material_expressions(data, "", custom, name)
texture = node(unreal.MaterialExpressionTextureObjectParameter, -650, 450)
texture.set_editor_property("parameter_name", "Positions")
texture.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
lib.connect_material_expressions(texture, "", custom, "Positions")
enabled = node(unreal.MaterialExpressionScalarParameter, -650, 650)
enabled.set_editor_property("parameter_name", "SimulationEnabled")
enabled.set_editor_property("default_value", 0.0)
lib.connect_material_expressions(enabled, "", custom, "Enabled")
lib.connect_material_property(custom, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
color = node(unreal.MaterialExpressionConstant3Vector, -150, -220)
color.set_editor_property("constant", unreal.LinearColor(0.025, 0.48, 0.85, 1))
lib.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, -150, -340)
rough.set_editor_property("r", 0.42)
lib.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
lib.recompile_material(material)
assets.save_loaded_asset(material)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
# Use an empty map, independent of the first-person template.
if assets.does_asset_exist(MAP):
    level.load_level(MAP)
    for old_actor in actors.get_all_level_actors():
        if old_actor.get_actor_label().startswith("CubeFlockDemo_"):
            actors.destroy_actor(old_actor)
else:
    level.new_level(MAP)

def spawn(cls, label, location, rotation=unreal.Rotator()):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), rotation)
    actor.set_actor_label("CubeFlockDemo_" + label)
    return actor

flock_cls = unreal.load_class(None, "/Script/CubeFlock.CubeFlockActor")
assert flock_cls, "CubeFlock plugin was not compiled/loaded"
flock = spawn(flock_cls, "2000_GPU_Cubes", (0, 0, 1000))
flock.set_editor_property("bird_count", 2000)
flock.set_editor_property("flock_material", material)
# Ensure construction script preview is refreshed after assigning the material.
flock.set_actor_location(unreal.Vector(0, 0, 1000), False, False)

floor = spawn(unreal.StaticMeshActor, "Floor", (0, 0, -30))
floor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube"))
floor.set_actor_scale3d(unreal.Vector(400, 400, 0.5))

sun = spawn(unreal.DirectionalLight, "Sun", (0, 0, 2500), unreal.Rotator(-45, -35, 0))
sun.light_component.set_editor_property("intensity", 4.0)
sky = spawn(unreal.SkyLight, "SkyLight", (0, 0, 1800))
sky.light_component.set_editor_property("intensity", 1.0)
sky.light_component.set_editor_property("real_time_capture", True)
spawn(unreal.SkyAtmosphere, "Atmosphere", (0, 0, 0))
start_position = unreal.Vector(2200, -2800, 2000)
look = unreal.MathLibrary.find_look_at_rotation(start_position, unreal.Vector(0, 0, 1000))
spawn(unreal.PlayerStart, "Start", (2200, -2800, 2000), look)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CubeFlock.CubeFlockDemoGameMode"))
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(start_position, look)
level.save_current_level()
assets.save_directory(ROOT)
unreal.log("CUBE_FLOCK_DEMO_CREATED " + MAP)



