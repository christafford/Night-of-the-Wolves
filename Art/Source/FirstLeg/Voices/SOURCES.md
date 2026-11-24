# First-leg dialogue

The six lines use the game's original English script. The hermit gives a low, gravelly warning through the timber, projects his threat on the porch and breaks into a frightened plea in the pit. The bandit speaks measured English with a northern Mexican accent. His low, smoother voice leaves a beat between commands as he sizes up Max and sees the gold. Both performances were synthesized locally from original written direction. [Qwen3-TTS VoiceDesign model card](https://huggingface.co/Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign)

Each speaker recorded all three lines in a single take. The authoring script stores the seeds, direction, edit points and mastering filters. The hermit's vowels have gentle saturation and pitch-preserving time stretch. The cabin window softens the warning, and the pit adds a short reflection. The roadside lines retain the pauses between the bandit's commands. [Dry performances](Performances) · [Authoring script](../../../../Scripts/render_first_leg_dialogue.py)

## Remaster and import

Run these commands from the project root with `UE_ROOT` and `UE_PROJECT` set as in [AGENTS.md](../../../../AGENTS.md). Mastering the saved takes requires Python and FFmpeg. Unreal imports the finished mono, 24 kHz WAVs under `/Game/NightOfTheWolves/FirstLeg/Voices`.

```sh
python3 Scripts/render_first_leg_dialogue.py
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PWD/Scripts/import_first_leg_dialogue.py" \
  -nullrhi -nosound -unattended -nop4
```

## Mouth timing and facial hair

The runtime samples five mouth controls at the playback position of each finished recording. To rebake after changing a line, install PocketSphinx 5.0.4 in a Python environment with FFmpeg on PATH, then run the following from the project root. The generated C++ data ships with the game. The game primes the short recordings and offsets Unreal’s playback cursor by the project’s `OutputLatency` setting in [DefaultGame.ini](../../../../Config/DefaultGame.ini). On Linux this accounts for the buffered audio measured at the output.

```sh
python Scripts/bake_dialogue_articulation.py
```

[The facial hair authoring script](../../../../Scripts/skin_dialogue_whiskers.py) binds the original beard and moustache cards to the moving face meshes. The wordless fall uses a separate recording. [Wilhelm source and credit](../Hermit/SOURCES.md)

## Audition another performance

The authoring environment on this workstation is `~/.cache/night-of-the-wolves/qwen-tts`. It uses Python 3.12.16, qwen-tts 0.1.1, PyTorch 2.8.0, Transformers 4.57.3 and SoX. The script pins the VoiceDesign checkpoint and writes new dry takes to the audition directory. After selecting an alternate take, update its edit points before mastering.

```sh
source "$HOME/.cache/night-of-the-wolves/qwen-tts/bin/activate"
export PATH="$HOME/.cache/night-of-the-wolves/sox/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/.cache/night-of-the-wolves/sox/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
python Scripts/render_first_leg_dialogue.py --audition /tmp/wolves-audition --actor Hermit --seed 204
```

The earlier prototype used Flite's kal16 voice for both characters. Its historical license remains in [Flite-LICENSE.txt](Flite-LICENSE.txt).
