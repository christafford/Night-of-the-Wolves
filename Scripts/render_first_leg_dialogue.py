"""Master the checked-in voice takes, or audition new performances using Qwen3-TTS."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import wave

VOICES = Path(__file__).resolve().parents[1] / "Art/Source/FirstLeg/Voices"
MODEL_ID = "Qwen/Qwen3-TTS-12Hz-1.7B-VoiceDesign"
REVISION = "5ecdb67327fd37bb2e042aab12ff7391903235d3"

# Keep the three beats in one recording so the same actor plays the entire scene.
CAST = {
    "Hermit": (
        173,
        "Stay away from my door!\n\nI warned you!\n\nGet me out of here!",
        "One older male frontier hermit, wiry and ferociously short-tempered. Western American "
        "English with a dry southwestern drawl. His high, pinched, gravelly rasp has a nasal "
        "twang; his pitch jumps when he loses his temper. Three separate lines of dialogue with "
        "a full silent beat between them. First he growls a warning through clenched teeth, "
        "stretching 'my door'. Then his temper snaps and he barks 'I warned you!' in one blow. "
        "Finally, trapped and furious, he yells for help; his voice cracks upward on 'out'. "
        "Start on the first word. Close, dry solo voice.",
    ),
    "Bandit": (
        2719,
        "Hands up! Don't move.\n\nThe bag. Put it down. Slowly.\n\nStay right there.",
        "One middle-aged Mexican outlaw speaking fluent, lightly Mexican-accented English. "
        "A lean, weathered, medium-low voice, a little breath and grain behind the teeth. "
        "He sounds amused at having the upper hand and can turn dangerous in a word. "
        "Three separate lines, a full silent beat between them. 'Hands up! Don't move' "
        "is a sharp, quick ambush. His voice warms with greedy interest on 'the bag', "
        "then he draws out 'slowly' as a quiet threat. 'Stay right there' is almost friendly, "
        "with a hard downward turn on 'there'. Intimate western film dialogue, dry solo voice.",
    ),
}

# Edit points in seconds in the approved dry takes. Keep the breath and release of each line.
# The warning is heard through timber; the pit has a short reflection below ground.
EDITS = {
    "Hermit": [
        ("HermitWarning", [(0.0, 1.53)], "highpass=f=90,lowpass=f=2600", -19),
        ("HermitShout", [(1.70, 2.73)], "highpass=f=80", -16),
        ("HermitPit", [(3.13, 4.28)], "highpass=f=105,lowpass=f=3400,aecho=0.90:0.95:57:0.16", -18),
    ],
    "Bandit": [
        ("BanditDemand", [(0.055, 0.88), (1.96, 2.92)], "highpass=f=78", -17),
        ("BanditGold", [(3.16, 3.99), (4.32, 5.35), (5.57, 6.72)], "highpass=f=78", -18),
        ("BanditStay", [(7.20, 8.47)], "highpass=f=78", -18),
    ],
}


def audition(directory, actors, seed):
    import soundfile as sf
    import torch
    from huggingface_hub import snapshot_download
    from qwen_tts import Qwen3TTSModel

    directory.mkdir(parents=True, exist_ok=True)
    model = Qwen3TTSModel.from_pretrained(
        snapshot_download(MODEL_ID, revision=REVISION), device_map="cuda:0",
        dtype=torch.bfloat16, attn_implementation="sdpa",
    )
    for actor in actors:
        default_seed, words, direction = CAST[actor]
        torch.manual_seed(default_seed if seed is None else seed)
        takes, rate = model.generate_voice_design(
            text=words, instruct=direction, language="English", max_new_tokens=256,
        )
        dest = directory / (actor + ".wav")
        sf.write(dest, takes[0], rate, subtype="PCM_24")
        print(f"{dest}  {len(takes[0]) / rate:.2f} seconds", flush=True)


def master():
    with tempfile.TemporaryDirectory(dir=VOICES) as scratch:
        for actor, lines in EDITS.items():
            take = VOICES / "Performances" / (actor + ".wav")
            with wave.open(str(take), "rb") as dry:
                rate = dry.getframerate()
                if dry.getnchannels() != 1 or dry.getsampwidth() != 3:
                    raise ValueError(f"Expected mono PCM24 in {take}")
                for name, cuts, effect, loudness in lines:
                    # Join in the quiet spaces, keeping the spoken cadence inside each phrase.
                    pcm = bytearray()
                    for start, end in cuts:
                        first, last = round(start * rate), round(end * rate)
                        if not 0 <= first < last <= dry.getnframes():
                            raise ValueError(f"{name} edit falls outside {take}")
                        dry.setpos(first)
                        pcm.extend(dry.readframes(last - first))
                    subprocess.run(
                        ["ffmpeg", "-nostdin", "-loglevel", "error", "-y", "-f", "s24le",
                         "-ar", str(rate), "-ac", "1", "-i", "pipe:0",
                         "-af", f"{effect},loudnorm=I={loudness}:TP=-2:LRA=8",
                         "-ar", "24000", "-c:a", "pcm_s16le", str(Path(scratch) / (name + ".wav"))],
                        input=pcm, check=True,
                    )
        for line in Path(scratch).glob("*.wav"):
            line.replace(VOICES / line.name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audition", type=Path, metavar="DIR", help="generate dry takes into DIR")
    parser.add_argument("--actor", choices=CAST, help="generate just this character's take")
    parser.add_argument("--seed", type=int, help="audition an alternative reading")
    args = parser.parse_args()
    if args.audition:
        audition(args.audition, [args.actor] if args.actor else CAST, args.seed)
    elif args.actor or args.seed is not None:
        parser.error("--actor and --seed apply to --audition")
    else:
        master()
