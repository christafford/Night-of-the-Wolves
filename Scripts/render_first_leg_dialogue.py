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
        583,
        "Stay away from my door.\n\nI warned you!\n\nGet me out of here!",
        "An old man alone in a timber cabin, speaking American English. Very low weathered "
        "bass-baritone, deep resonant chest voice with a rough creaking edge on sustained "
        "vowels. He speaks very slowly and distinctly. The quiet restraint in his first "
        "warning is frightening. Give 'away' and 'my door' weight. Pause to breathe after "
        "the warning. 'I warned you!' is projected directly at the intruder: a slow, "
        "deliberate shout, heavy stress on 'warned'. Pause again. Finally he loses his "
        "footing and calls for help, fear breaking through the gravel on 'out of here'. "
        "Three separate spoken lines in one voice, a full silent beat between them. Dry solo dialogue.",
    ),
    "Bandit": (
        1459,
        "Hands up. Don't move.\n\nThe bag. Put it down. Slowly.\n\nStay right there.",
        "A middle-aged man from northern Mexico. Spanish is his first language. Deliver "
        "the English script in his natural, clearly audible northern Mexican accent: "
        "pure rounded vowels, even syllables, clear consonants. Low, warm baritone, "
        "smooth and resonant with a little smoke in the release. His authority is calm. "
        "He takes his time. 'Hands up' is a firm command; let a beat pass before "
        "'don't move'. Seeing the gold, he savors 'the bag', pauses, then coaxes the "
        "traveler to put it down, lingering over 'slowly'. A full beat, then "
        "'stay right there' in a near-intimate, suddenly cold voice. Spacious pauses "
        "between all three lines. Dry solo film dialogue, speak only the script.",
    ),
}

# The hermit's mids carry the rasp through timber. A little saturation thickens
# the louder vowels, and pitch-preserving time stretch leaves room for his delivery.
HERMIT_TONE = "highpass=f=62,asoftclip=type=tanh:threshold=0.30:oversample=4"

# Edit in breaths between lines and retain the bandit's pauses inside each command.
EDITS = {
    "Hermit": [
        ("HermitWarning", 0.0, 2.51, HERMIT_TONE + ",atempo=.89,lowpass=f=2800", -19),
        ("HermitShout", 2.73, 3.89, HERMIT_TONE + ",atempo=.79", -16),
        ("HermitPit", 4.01, 5.12, HERMIT_TONE + ",atempo=.82,lowpass=f=3400,aecho=0.90:0.95:57:0.16", -18),
    ],
    "Bandit": [
        ("BanditDemand", 0.0, 3.08, "highpass=f=66", -17),
        ("BanditGold", 3.59, 7.92, "highpass=f=66", -18),
        ("BanditStay", 8.43, 10.20, "highpass=f=66", -18),
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
                for name, start, end, effect, loudness in lines:
                    first, last = round(start * rate), round(end * rate)
                    if not 0 <= first < last <= dry.getnframes():
                        raise ValueError(f"{name} edit falls outside {take}")
                    dry.setpos(first)
                    pcm = dry.readframes(last - first)
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
