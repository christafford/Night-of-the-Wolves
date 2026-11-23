# Coastal forest

The opening uses mature fir and shore-pine silhouettes, young firs, fern beds and fallen timber to enclose the trail. Bent, textured branch cards form the evergreen crowns. The surface changes from damp earth and embedded gravel at the center to needle litter under the ferns. A pair of raised banks follows the pursuit spline and opens out around the cottage. These are art-directed Pacific Northwest silhouettes. The photographic sources are temperate conifers and ferns.

The foreground is baked foliage. Select one of the **Cascades nursery** volumes in the editor and use **Resimulate** to regrow its stand from the **PFS_CoastalStand** asset. Its four foliage types set species, seed density, shade competition, slope limits and spread. Saplings and ferns can grow beneath taller crowns. A separate set of fern clusters follows the trail's outside edges. Player collision uses narrow trunk capsules. Small plants and fallen timber are decorative.

The outer woods use **PCG_RidgeCanopy**. Its grid jitters position, yaw and scale, projects the trees onto terrain using a world ray query, and spawns weighted fir and pine instances on both sides of the route. The through-trail, cottage yard and creek crossing have their own placement space. Use the graph and foliage types to substitute equivalent Fab assets after importing them into the project. Check their pivots, scale, opacity masks, trunk collision and culling at both the player camera and cabin sightline.

The fern cull range is 33–60 m, young fir 55–92 m, mature trees 130–185 m and outer PCG canopy 160–235 m. A photographed night sky puts the moon beyond the trees. Volumetric fog covers 72 m and distance fog supplies the deeper silhouettes. Short draw distances and shared static meshes keep small plants affordable. The 60 FPS target is a tuning objective across the complete route.

On the November 22 opening run, Linux Shipping at 1920×1080 Epic averaged **92.64 FPS** on an i5-13400F and RTX 3080 using GPU-accelerated Weston/Xwayland. Of 1,745 presented frames, 1,730 met the 16.67 ms budget. The same opening on a TigerVNC display averaged about 30 FPS. Both measurements cover the roughly 19-second trailhead-to-cottage approach. Later encounters and other hardware still need profiling. The local [frame-time report](../Saved/FirstLegReview/Frametimes/README.md) includes every sample and the display configurations.

## Rebuild the assets

The saved map and materials are ready for gameplay. The authoring scripts run against the checked-in map, with the route actor and existing cabin in place. The runtime uses the cooked meshes, materials, baked foliage instances and PCG graph.

1. Run `python3 Scripts/forest/prepare_models.py` from the project with Pillow and curl installed to regenerate the source meshes and atlases. The source art is already under `Art/Source/Cascades`.
2. With the editor closed, run the following from the project root. The planting passes use the full editor with NullRHI because foliage simulation traces against the editor physics scene. After changing source meshes or atlases, set `NW_REIMPORT_FOREST=1` for the import step before planting again.

```sh
UE_ROOT="$HOME/UnrealEngine-5.9-preview-2"
UE_PROJECT="$PWD/Night_of_the_Wolves.uproject"
for script in import_cascades woodland_surfaces prepare_foliage_shaders moonlit_sky cottage_ground; do
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
    -run=pythonscript "-script=$PWD/Scripts/forest/$script.py" -nullrhi -unattended -nop4
done
for script in plant_cascades cottage_backdrop creek_banks; do
  "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$UE_PROJECT" \
    "-ExecutePythonScript=$PWD/Scripts/forest/$script.py" \
    -nullrhi -unattended -nop4 -EnablePlugins=GeometryScripting \
    '-ini:EditorPerProjectUserSettings:[/Script/UnrealEd.EditorExperimentalSettings]:bProceduralFoliage=True'
done
```

The planting scripts replace the generated nursery stands, verge clusters, logs and the trees behind the cottage. During the first-leg pass, resimulating 30 nursery volumes produced 1,123 trees of all ages and 2,310 ferns. The baked edge layer added 937 ferns, 30 fallen logs and 96 trees behind the clearing. A standalone editor-game run independently generated 71 firs and 45 pines from the ridge PCG graph. The graph, materials and foliage types are editable in the Content Browser under `NightOfTheWolves/Cascades`. Changing the spline geometry requires regenerating the bank mesh to fit.

The cottage turnoff and through-trail share one terrain mesh, `SM_CottageClearing`. Its ground material paints both treads into the needle litter. The 190 × 320 cm opening reaches the cottage threshold and extends under both jambs. A separate tagged cover closes it until Max digs. The lintel and recessed entrance ceiling provide 324 cm of clearance for the jump. After editing the approach or opening, rebuild the clearing with `Scripts/forest/cottage_ground.py`.

The creek uses a separate continuous mesh with irregular banks on both sides. Surface UVs follow the slope, so the shingle at the water line and soil below the root shelf keep their physical scale. Exposed roots, water-worn stones, fern beds and fallen timber line the channel. A wooded rise behind the bandit keeps the last visible bend open. The shipping game uses the established crossing collision, anchor, water checks and checkpoints. The creek art is instantiated by `AFirstLegCrossing` at play time.

## Source art

Poly Haven releases these assets under CC0. The current build uses project-authored, compact conifer meshes with new branch atlases assembled from the photographic twig and bark maps in [Fir Tree 01](https://polyhaven.com/a/fir_tree_01) and [Pine Tree 01](https://polyhaven.com/a/pine_tree_01). The fern and fallen timber derive from one mesh variant of [Fern 02](https://polyhaven.com/a/fern_02) and [Dead Tree Trunk 02](https://polyhaven.com/a/dead_tree_trunk_02). The forest floor uses the existing Poly Haven ground, mud and stone textures credited in `Art/Source/Presentation/SOURCES.md`.

The moonlit sky uses a 2K reduction of the tone-mapped panorama [Qwantani Moonrise (Pure Sky)](https://polyhaven.com/a/qwantani_moonrise_puresky), also CC0. Only the sky is visible above the authored forest.

Three normalized rock variants share color and normal maps from [Rock Moss Set 01](https://polyhaven.com/a/rock_moss_set_01). The exposed banks use the gravel color and normal maps from [Dry Riverbed Rock](https://polyhaven.com/a/dry_riverbed_rock). Both photographs and meshes are CC0 from Poly Haven.
