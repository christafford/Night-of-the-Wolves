"""Render No Birds at This Hour, an original looping string cue for the forest pursuit."""
from functools import lru_cache
from pathlib import Path
import json
import subprocess
import urllib.parse
import urllib.request

import numpy as np
import soundfile as sf
from scipy import signal

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "Intermediate/ScoreSamples"
MASTER = ROOT / "Art/Source/Music/SW_MoonlitPursuit.wav"
REV = "440300901dfe9275fd84e0b7763af1f8443ae62e"
RATE = 48000
SECONDS = 76
FRAMES = SECONDS * RATE
TARGET_LUFS = -21.0

# MIDI concert pitch. VSCO labels concert D3 (MIDI 50) as D2.
SAMPLES = {
    "bass": ("Strings/Solo Contrabass/SusNV/BKCtbss_SusNV_D1_v1_rr1.wav", 38),
    "cello": ("Strings/Cello Section/susvib/susvib_D2_v1_1.wav", 50),
    "viola": ("Strings/Viola Section/susvib/ViolaEns_susvib_D3_v1_1.wav", 62),
    "tremolo": ("Strings/Viola Section/trem/Violas_trem_A3_v1_rr1.wav", 69),
    "drum": ("VSCO 1 Percussion/drums/bass/bdrum_muted_pp_1.wav", 0),
    "metal": ("VSCO 1 Percussion/varMetal/Cymbals/susp/susp_hit_softmall_roll2_cresc.wav", 0),
}

# A solo bass holds beneath the changes of bow in the cello desk. The rests
# after each falling phrase leave the footfalls and wolf calls in the clear.
# Times are seconds; each entry gives onset, concert pitch, bow length and level.
BASS = [
    (-2.3, 38, 11.1, .55), (8.2, 38, 8.9, .42), (17.6, 34, 7.8, .54),
    (25.2, 33, 9.2, .46), (34.0, 38, 11.6, .67), (46.0, 39, 7.1, .47),
    (53.0, 35, 8.4, .53), (61.2, 32, 7.4, .60), (69.0, 33, 4.8, .41),
]
CELLO = [
    (-1.0, 46, 4.7, .44), (3.4, 45, 6.1, .51), (10.5, 50, 5.2, .49),
    (15.8, 51, 4.1, .41), (19.9, 50, 5.3, .61), (25.4, 43, 5.5, .33),
    (34.0, 46, 5.5, .67), (39.4, 45, 7.2, .61), (47.0, 51, 5.6, .50),
    (52.3, 50, 6.4, .64), (61.2, 44, 5.2, .57), (66.7, 45, 5.9, .42),
]
VIOLA = [
    (4.2, 57, 4.5, .15), (12.2, 58, 5.9, .26), (18.6, 57, 5.3, .31),
    (27.1, 56, 4.4, .24), (35.4, 57, 6.8, .31), (42.2, 56, 4.3, .34),
    (48.6, 58, 5.0, .31), (55.0, 57, 4.6, .25), (62.1, 62, 6.6, .37),
    (69.2, 61, 4.3, .18),
]
TREM = [
    (7.5, 69, 6.1, .068), (15.1, 70, 7.8, .10), (23.3, 69, 4.8, .055),
    (34.1, 68, 8.2, .13), (42.5, 69, 5.7, .088), (53.9, 70, 7.4, .11),
    (61.6, 68, 8.0, .16), (71.4, 69, 6.1, .064),
]


@lru_cache(maxsize=None)
def recording(section):
    name, _ = SAMPLES[section]
    path = CACHE / Path(name).name
    if not path.exists():
        CACHE.mkdir(parents=True, exist_ok=True)
        url = f"https://raw.githubusercontent.com/sgossner/VSCO-2-CE/{REV}/" + urllib.parse.quote(name)
        with urllib.request.urlopen(url) as response:
            data = response.read()
        pending = path.with_suffix(".download")
        pending.write_bytes(data)
        pending.replace(path)

    wave, rate = sf.read(path, dtype="float32", always_2d=True)
    peak = np.max(np.abs(wave))
    onset = np.flatnonzero(np.max(np.abs(wave), axis=1) > peak * .012)[0]
    wave = wave[max(0, onset - round(rate * .025)):] / peak
    return wave, rate


def add_circular(mix, start, audio):
    first = round(start * RATE) % FRAMES
    cut = min(len(audio), FRAMES - first)
    mix[first:first + cut] += audio[:cut]
    if cut < len(audio):
        mix[:len(audio) - cut] += audio[cut:]


def play(mix, section, events, pan, attack, release, cutoff):
    source, rate = recording(section)
    _, root = SAMPLES[section]
    lowpass = signal.butter(2, cutoff, fs=RATE, output="sos")
    source_index = np.arange(len(source))
    balance = np.sqrt((1. - pan, 1. + pan))
    for start, key, duration, gain in events:
        step = rate / RATE * 2 ** ((key - root) / 12.)
        count = min(round(duration * RATE), int((len(source) - 1) / step))
        read_at = np.arange(count) * step
        voice = np.column_stack([np.interp(read_at, source_index, side) for side in source.T])
        voice = signal.sosfilt(lowpass, voice, axis=0)
        t = np.arange(count) / RATE
        # Use the recorded motion between bow changes. Shape just the entries and
        # releases, so one player can come up through another in the held dyads.
        shape = np.sin(np.clip(t / attack, 0., 1.) * np.pi / 2.) ** 2
        shape *= np.sin(np.clip((count / RATE - t) / release, 0., 1.) * np.pi / 2.) ** 2
        mid = voice.mean(axis=1, keepdims=True)
        voice = mid + (voice - mid) * .60
        add_circular(mix, start, voice * balance * (gain * shape[:, None]))


def score():
    mix = np.zeros((FRAMES, 2), dtype="float32")
    play(mix, "bass", BASS, .02, .64, 1.05, 1180)
    play(mix, "cello", CELLO, -.26, .72, .86, 2280)
    play(mix, "viola", VIOLA, .30, 1.15, 1.35, 2350)
    play(mix, "tremolo", TREM, -.10, 1.8, 1.5, 2490)

    # A handful of widely separated skin strokes underneath the bows. The
    # sustained dissonance has room to grow before the next stroke arrives.
    strokes = [(1.55, -4, 1.55, .60), (9.60, -7, 1.6, .36),
               (19.90, -5, 1.55, .69), (31.40, -8, 1.65, .46),
               (34.80, -4, 1.50, .78), (44.20, -6, 1.60, .53),
               (55.15, -5, 1.6, .52), (62.30, -4, 1.55, .73),
               (69.20, -7, 1.65, .37)]
    play(mix, "drum", strokes, -.08, .008, .35, 740)
    play(mix, "metal", [(23.8, -16, 6.0, .064), (58.3, -18, 6.8, .092)],
         .43, 2.4, 2.0, 1700)

    # Fold the room's decay over the join, alongside the opening bass bow.
    rng = np.random.default_rng(235)
    dry = mix.copy()
    t = np.arange(round(1.70 * RATE)) / RATE
    for side in range(2):
        air = signal.sosfilt(signal.butter(2, 2400, fs=RATE, output="sos"), rng.normal(size=len(t)))
        air *= np.exp(-3.7 * t) * np.clip((t - .031) / .055, 0., 1.)
        air /= np.linalg.norm(air)
        reflected = signal.oaconvolve(dry[:, side] + dry[:, 1-side] * .18, air.astype("float32"))
        tail = len(reflected) - FRAMES
        reflected[:tail] += reflected[FRAMES:]
        mix[:, side] += reflected[:FRAMES] * .15
    return mix


def main():
    mix = score()
    meter = subprocess.run(
        ["ffmpeg", "-hide_banner", "-f", "f32le", "-ar", str(RATE), "-ac", "2", "-i", "pipe:",
         "-af", f"loudnorm=I={TARGET_LUFS}:TP=-2:LRA=9:print_format=json", "-f", "null", "-"],
        input=mix.astype("<f4").tobytes(), capture_output=True, check=True)
    stats = json.loads(meter.stderr[meter.stderr.rfind(b"{"):meter.stderr.rfind(b"}") + 1])
    gain = min(TARGET_LUFS - float(stats["input_i"]), -2. - float(stats["input_tp"]))
    MASTER.parent.mkdir(parents=True, exist_ok=True)
    sf.write(MASTER, mix * 10**(gain / 20), RATE, subtype="PCM_16")
    print(f"{MASTER.name}   {SECONDS}s   {float(stats['input_i']) + gain:.2f} LUFS")


if __name__ == "__main__":
    main()
