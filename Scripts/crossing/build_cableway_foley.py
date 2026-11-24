"""Render and import the old logging trolley's three original mechanical sounds in Unreal Editor."""
import array
import math
import os
import random
import wave

import unreal


RATE = 32000
DEST = "/Game/NightOfTheWolves/Crossing/Foley"
FOLDER = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_intermediate_dir()), "CablewayFoley")
os.makedirs(FOLDER, exist_ok=True)


def render(name, duration):
    rng = random.Random(294)
    out = array.array("h")
    fast = slow = 0.0
    for i in range(round(RATE * duration)):
        t = i / RATE
        white = rng.uniform(-1, 1)
        fast = .59 * fast + .41 * white
        slow = .973 * slow + .027 * white
        if name == "SheaveRoll":
            # A rolling bearing ticks once per turn over the finer rasp of wire strands.
            turns = t * 5
            tick = math.exp(-32 * (turns % 1))
            rasp = .42 + .15 * math.cos(math.tau * turns)
            ring = math.sin(math.tau * 465 * t) + .45 * math.sin(math.tau * 745 * t)
            value = (fast - slow) * rasp + tick * (.25 * ring + .35 * fast) + .30 * slow
        else:
            if name == "ShackleClink":
                modes = ((836, .34, 19), (1463, .24, 22), (2391, .13, 27))
                struck = ((0, 1), (.076, .52))
            else:
                modes = ((91, .49, 16), (231, .32, 14), (483, .22, 21), (1044, .14, 20))
                struck = ((0, 1), (.112, .54), (.193, .21))
            value = 0
            for start, strength in struck:
                age = t - start
                if age < 0:
                    continue
                tone = sum(level * math.sin(math.tau * hz * age) * math.exp(-rate * age)
                           for hz, level, rate in modes)
                value += strength * (tone + .70 * fast * math.exp(-65 * age)) * min(1, age * 650)
        out.append(round(32767 * min(.94, max(-.94, value))))
    filename = os.path.join(FOLDER, name + ".wav")
    with wave.open(filename, "wb") as sound:
        sound.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        sound.writeframes(out.tobytes())
    task = unreal.AssetImportTask()
    task.set_editor_properties(dict(filename=filename, destination_path=DEST, automated=True,
                                   replace_existing=True, save=True))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(DEST + "/" + name)
    if not sound:
        raise RuntimeError("Failed to import " + name)
    sound.set_editor_property("looping", name == "SheaveRoll")
    unreal.EditorAssetLibrary.save_loaded_asset(sound, only_if_is_dirty=False)


render("SheaveRoll", 1.6)
render("ShackleClink", .5)
render("EndStop", .7)
unreal.log("CABLEWAY_FOLEY_READY")
