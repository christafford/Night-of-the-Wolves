# Hermit's fall

The fall uses the familiar Wilhelm take from the Sunset Editorial collection at USC Cinema. Craig Smith digitized the reel and published it under CC0 1.0. The game uses 1.36 seconds starting at 27.238 in his lossless master. A 75 Hz high-pass removes low-frequency noise and 6 ms and 125 ms fades smooth its edges. The finished mono WAV measures −15.3 LUFS and −6.2 dBTP. [Archivist’s source and license](https://freesound.org/people/craigsmith/sounds/675810/) · [Preservation notes](https://blog.freesound.org/?p=1515) · [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)

The master on the Internet Archive has SHA-1 `76d3ccb36da6655ee551abcf0407424266d23bea`. To remaster and import, run the following from the project root with the engine and project variables set as in [AGENTS.md](../../../../AGENTS.md).

```sh
curl -fL 'https://archive.org/download/SSE_Library_VOICES/SCREAM/VOXScrm_Man%20eaten%20by%20alligator%3B%20screams%20%5BWilhelm_CS_USC.wav' -o /tmp/wilhelm-session.wav
ffmpeg -y -ss 27.238 -i /tmp/wilhelm-session.wav -t 1.36 \
  -af 'highpass=f=75,volume=-3.6dB,afade=t=in:d=0.006,afade=t=out:st=1.235:d=0.125' \
  -ar 24000 -ac 1 -c:a pcm_s16le -fflags +bitexact -flags:a +bitexact -map_metadata -1 \
  Art/Source/FirstLeg/Hermit/SC_HermitFallYell.wav
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$UE_PROJECT" \
  -run=pythonscript -script="$PWD/Scripts/hermit/import_fall_yell.py" -nullrhi -nosound -unattended -nop4
```
