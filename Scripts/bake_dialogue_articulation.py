"""Bake mouth motion from the six final mono WAVs. Requires PocketSphinx 5.0.4 and FFmpeg."""
import array
import bisect
import math
from pathlib import Path
import statistics
import subprocess
import wave

from pocketsphinx import Decoder

PROJECT = Path(__file__).resolve().parents[1]
VOICES = PROJECT / "Art/Source/FirstLeg/Voices"
OUTPUT = PROJECT / "Source/Night_of_the_Wolves/DialogueArticulationData.inl"
FPS = 50

# Separate emphatic clauses at the middle of their recorded pauses, so the
# aligner cannot stretch a consonant over the silence between two commands.
CLAUSES = {
    "HermitWarning": [(0., "stay away from my door")],
    "HermitShout": [(0., "i warned you")],
    "HermitPit": [(0., "get me out of here")],
    "BanditDemand": [(0., "hands up"), (.82, "don't move")],
    "BanditGold": [(0., "the bag"), (.82, "put it down"), (1.85, "slowly")],
    "BanditStay": [(0., "stay right there")],
}

# Jaw aperture, lip rounding, lip spread, bilabial seal, and lower lip to teeth.
# Separate vowel targets give diphthongs a visible movement within one phone.
REST = (0., 0., 0., 0., 0.)
SHAPES = {
    "SIL": REST,
    "AA": (.95, .02, .19, 0., 0.),
    "AE": (.76, .0, .68, 0., 0.),
    "AH": (.65, .03, .15, 0., 0.),
    "AO": (.70, .53, .0, 0., 0.),
    "EH": (.51, .0, .61, 0., 0.),
    "IH": (.31, .0, .67, 0., 0.),
    "IY": (.22, .0, .93, 0., 0.),
    "UH": (.34, .64, .0, 0., 0.),
    "UW": (.21, .95, .0, 0., 0.),
}
for names, shape in [
    ("M B P", (.01, 0., .06, 1., 0.)),
    ("F V", (.16, .0, .3, .0, .92)),
    ("S Z", (.11, .0, .62, .0, .0)),
    ("T D N", (.23, .0, .24, .0, .0)),
    ("G K", (.40, .0, .15, .0, .0)),
    ("TH DH", (.26, .0, .31, .0, .0)),
    ("L", (.30, .0, .31, .0, .0)),
    ("R", (.23, .48, .0, .0, .0)),
    ("W", (.14, .95, .0, .0, .0)),
    ("Y", (.21, .0, .68, .0, .0)),
    ("HH", (.42, .0, .16, .0, .0)),
]:
    for name in names.split():
        SHAPES[name] = shape
VOWEL_MOVES = {"AY": ("AA", "IH"), "AW": ("AA", "UH"),
               "EY": ("EH", "IY"), "OW": ("AO", "UW")}


def clamp(n):
    return min(1., max(0., n))


def acoustic_alignment(path, clauses):
    pcm = subprocess.check_output(
        ["ffmpeg", "-nostdin", "-loglevel", "error", "-i", str(path),
         "-f", "s16le", "-ac", "1", "-ar", "16000", "-"]
    )
    phones, words = [], []
    ends = [start for start, _ in clauses[1:]] + [len(pcm) / 32000.]
    for (start, transcript), end in zip(clauses, ends):
        region = pcm[round(start * 16000) * 2:round(end * 16000) * 2]
        decoder = Decoder(lm=None, loglevel="ERROR")
        decoder.set_align_text(transcript)
        for second_pass in (False, True):
            if second_pass:
                decoder.set_alignment()
            decoder.start_utt()
            decoder.process_raw(region, full_utt=True)
            decoder.end_utt()
        found = []
        for word in decoder.get_alignment():
            if not word.name.startswith("<"):
                base = word.name.split("(")[0]  # Pronunciation alternatives in CMUDict
                words.append((base, start + word.start / 100.))
                found.append(base)
            for phone in word:
                a = start + phone.start / 100.
                b = a + phone.duration / 100.
                # A stop's acoustic alignment can include the quiet gap before
                # its burst. Limit the anticipatory seal to the final 90 ms.
                if phone.name in ("B", "P"):
                    a = max(a, b - .09)
                phones.append((a, b, phone.name))
        if found != transcript.split():
            raise ValueError(f"Misaligned {path.name}: {found} for {transcript}")
    return phones, words


def bake(path, clauses):
    phones, words = acoustic_alignment(path, clauses)
    starts = [a for a, _, _ in phones]
    with wave.open(str(path), "rb") as wav:
        if wav.getsampwidth() != 2 or wav.getnchannels() != 1:
            raise ValueError(f"Expected mono PCM16 in {path}")
        rate = wav.getframerate()
        samples = array.array("h", wav.readframes(wav.getnframes()))
    duration = len(samples) / rate

    # A 20 ms RMS window measures syllable stress independently in each mix.
    power = [0]
    for value in samples:
        power.append(power[-1] + value * value)

    def rms(t):
        lo = min(len(samples), max(0, round((t - .01) * rate)))
        hi = min(len(samples), max(0, round((t + .01) * rate)))
        return math.sqrt((power[hi] - power[lo]) / max(1, hi - lo)) / 32768.

    levels = [rms(i / 100.) for i in range(math.ceil(duration * 100))]
    strong = statistics.quantiles(levels, n=10)[-1]

    def articulate(t):
        i = bisect.bisect_right(starts, t) - 1
        if i < 0:
            return REST
        a, b, phone = phones[i]
        if t >= b or phone == "SIL":
            return REST
        if phone in VOWEL_MOVES:
            first, last = VOWEL_MOVES[phone]
            blend = clamp(((t - a) / (b - a) - .18) / .70)
            shape = tuple(v + (w - v) * blend for v, w in zip(SHAPES[first], SHAPES[last]))
        else:
            shape = SHAPES[phone]
        jaw, round_lips, wide, seal, teeth = shape
        if seal:
            return shape
        level = rms(t)
        gate = clamp(level / .012)
        stress = math.sqrt(clamp(level / strong))
        return (jaw * (.40 + .60 * stress) * gate, round_lips * gate,
                wide * gate, seal, teeth * gate)

    frames = []
    for i in range(math.ceil(duration * FPS) + 1):
        t = i / FPS
        # Coarticulate across neighboring phones, with a small visual lead into
        # the next sound. Stops must reach contact even when they last just 30 ms.
        poses = [articulate(t + offset) for offset in (-.025, -.005, .015, .035)]
        pose = [sum(v*w for v, w in zip(channel, (1, 3, 3, 1))) / 8.
                for channel in zip(*poses)]
        pose[3] = max(pose[3], poses[1][3], poses[2][3])
        pose[0] *= 1. - .96 * pose[3]
        frames.append(tuple(round(255 * clamp(v)) for v in pose))
    frames[0] = frames[-1] = (0,) * 5
    return duration, frames, words, phones


def main():
    out = ["// Generated by Scripts/bake_dialogue_articulation.py from the mastered dialogue.",
           f"constexpr float MouthFramesPerSecond = {FPS}.f;", ""]
    durations = {}
    for clip, clauses in CLAUSES.items():
        duration, frames, words, phones = bake(VOICES / (clip + ".wav"), clauses)
        durations[clip] = duration
        print(clip, " ".join(f"{ph}@{a:.2f}" for a, _, ph in phones))
        out.append("// Word onsets (s): " + ", ".join(f"{w} {t:.2f}" for w, t in words))
        out.append(f"constexpr FPackedMouthPose {clip}Frames[] = {{")
        values = ["{" + ",".join(map(str, frame)) + "}" for frame in frames]
        for i in range(0, len(values), 6):
            out.append("\t" + ", ".join(values[i:i + 6]) + ",")
        out.extend(["};", ""])
    out.extend(["FMouthTrack GetMouthTrack(EDialogueLine Line)", "{", "\tswitch (Line)", "\t{"])
    for clip, duration in durations.items():
        out.extend([f"\tcase EDialogueLine::{clip}:",
                    f"\t\treturn {{{clip}Frames, UE_ARRAY_COUNT({clip}Frames), {duration:.6f}f}};"])
    out.extend(["\tdefault:", "\t\treturn {};", "\t}", "}", ""])
    OUTPUT.write_text("\n".join(out))


if __name__ == "__main__":
    main()
