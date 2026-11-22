"""Create the editable PCG woodland graph and volumes around the opening route.

Run using UnrealEditor-Cmd -run=pythonscript; generated gameplay does not use Python.
Existing M6 prototype trees remain as a conservative collision/visual fallback.
"""
import math
import unreal

DEST = '/Game/NightOfTheWolves/ForestTrees'
tools = unreal.AssetToolsHelpers.get_asset_tools()
assets = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

graph = assets.load_asset(DEST + '/PCG_Forest')
if graph:
    assets.delete_asset(DEST + '/PCG_Forest')
graph = tools.create_asset('PCG_Forest', DEST, unreal.PCGGraph, unreal.PCGGraphFactory())
grid, grid_settings = graph.add_node_of_type(unreal.PCGCreatePointsGridSettings)
grid_settings.set_editor_property('grid_extents', unreal.Vector(400, 400, 0))
grid_settings.set_editor_property('cell_size', unreal.Vector(400, 400, 100))
grid_settings.set_editor_property('coordinate_space', unreal.PCGCoordinateSpace.ORIGINAL_COMPONENT)
transform, tr = graph.add_node_of_type(unreal.PCGTransformPointsSettings)
tr.set_editor_property('offset_min', unreal.Vector(-120, -120, 0))
tr.set_editor_property('offset_max', unreal.Vector(120, 120, 0))
tr.set_editor_property('rotation_min', unreal.Rotator(0, 0, 0))
tr.set_editor_property('rotation_max', unreal.Rotator(0, 360, 0))
tr.set_editor_property('scale_min', unreal.Vector(1.4, 1.4, 1.4))
tr.set_editor_property('scale_max', unreal.Vector(2.1, 2.1, 2.1))
tr.set_editor_property('uniform_scale', True)
spawn, sp = graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings)
selector = sp.get_editor_property('mesh_selector_parameters')
entries = []
# Start with the imported low-poly tree; the scanned tree needs material work
# and its enormous source mesh would make an untested dense forest expensive.
for path, weight in [(DEST + '/dead_tree', 1)]:
    mesh = assets.load_asset(path)
    if not mesh:
        raise RuntimeError('Missing tree: ' + path)
    entry = unreal.PCGMeshSelectorWeightedEntry()
    descriptor = entry.get_editor_property('descriptor')
    descriptor.set_editor_property('static_mesh', mesh)
    entry.set_editor_property('descriptor', descriptor)
    entry.set_editor_property('weight', weight)
    entries.append(entry)
selector.set_editor_property('mesh_entries', entries)
graph.add_edge(grid, 'Out', transform, 'In')
graph.add_edge(transform, 'Out', spawn, 'In')
graph.add_edge(spawn, 'Out', graph.get_output_node(), 'Out')
assets.save_loaded_asset(graph)

if not levels.load_level('/Game/NightOfTheWolves/Maps/M1_Trail'):
    raise RuntimeError('Failed to load trail')
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('PCG forest patch '):
        actors.destroy_actor(actor)
route = next(a for a in actors.get_all_level_actors() if a.get_actor_label() == 'M5 pursuit route')
spline = route.spline
length = spline.get_spline_length()
distance = 0.0
count = 0
while distance < length:
    p = spline.get_location_at_distance_along_spline(distance, unreal.SplineCoordinateSpace.WORLD)
    direction = spline.get_direction_at_distance_along_spline(distance, unreal.SplineCoordinateSpace.WORLD)
    n = math.hypot(direction.x, direction.y)
    if n < .01:
        distance += 1600
        continue
    for sign in (-1, 1):
        # Avoid the cabin yard and its interaction space.
        if sign == 1 and 300 < p.x < 3900:
            continue
        offset = 1950 * sign
        center = unreal.Vector(p.x - direction.y/n*offset, p.y + direction.x/n*offset, p.z)
        volume = actors.spawn_actor_from_class(unreal.PCGVolume, center)
        volume.set_actor_label('PCG forest patch %02d %d' % (count, sign))
        comp = volume.get_editor_property('pcg_component')
        comp.set_graph(graph)
        comp.set_editor_property('seed', count*13 + (3 if sign == 1 else 7))
    count += 1
    distance += 1600
if not levels.save_current_level():
    raise RuntimeError('Could not save PCG volumes')
unreal.log('PCG_FOREST_DONE samples=%d length=%.0f' % (count, length))
