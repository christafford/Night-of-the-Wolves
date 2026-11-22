"""Render the wolf's unspatialized running breath in mono PCM using Python 3."""

import math
import sys
import wave
from array import array
from pathlib import Path
from random import Random

RATE = 48_000
# The opening camera and the wolf's stride use the same cycles per second.
STRIDES_PER_SECOND = 2.65
STRIDES = 53
TAU = 2 * math.pi
OUTPUT = Path(__file__).resolve().parents[2] / "Art/Source/Presentation/WolfPOVRunningBreath.wav"


class Resonance:
    """A band of turbulent airflow shaped by a mouth or nasal cavity."""

    def __init__(self, hz, q):
        omega = TAU * hz / RATE
        alpha = math.sin(omega) / (2 * q)
        self.gain = alpha / (1 + alpha)
        self.a1 = 2 * math.cos(omega) / (1 + alpha)
        self.a2 = (alpha - 1) / (1 + alpha)
        self.old_input = self.older_input = self.old_output = self.older_output = 0.0

    def sample(self, air):
        y = (
            self.gain * (air - self.older_input)
            + self.a1 * self.old_output
            + self.a2 * self.older_output
        )
        self.older_input, self.old_input = self.old_input, air
        self.older_output, self.old_output = self.old_output, y
        return y


def puff(phase, start, crest, end):
    if phase <= start or phase >= end:
        return 0.0
    if phase < crest:
        return math.sin((phase - start) / (crest - start) * math.pi * 0.5) ** 1.65
    return math.cos((phase - crest) / (end - crest) * math.pi * 0.5) ** 2.2


def render():
    rand = Random(30979)
    audio = array("f")
    slow_noise = 0.0
    flutter_hz, glottis = 88.0, 0.0
    for stride in range(STRIDES):
        # Every cycle occupies exactly one gait period. Strength, articulation, formants
        # and the tiny lip reopening vary independently across the longer phrase.
        frames = round((stride + 1) * RATE / STRIDES_PER_SECOND) - round(
            stride * RATE / STRIDES_PER_SECOND
        )
        effort = 0.91 + 0.10 * math.sin(stride * 0.67) + rand.uniform(-0.12, 0.12)
        mouth = rand.uniform(0.87, 1.10)
        throat = Resonance(rand.uniform(220, 282), 0.74)
        open_mouth = Resonance(571 * mouth, 0.90)
        exhale_air = Resonance(1205 * mouth, 0.81)
        muzzle = Resonance(2270 * mouth, 0.85)
        nostril = Resonance(rand.uniform(831, 1017), 1.19)
        inspiration = Resonance(rand.uniform(2090, 2630), 0.82)
        fine_air = Resonance(4470, 0.70)
        wet = Resonance(rand.uniform(1130, 1400), 0.83)
        crest = rand.uniform(0.19, 0.249)
        intake = rand.uniform(0.626, 0.686)
        expiration_end = rand.uniform(0.48, 0.54)
        breath_length = rand.uniform(0.934, 0.983)
        intake_strength = rand.uniform(0.47, 0.66)
        flutter = rand.uniform(0.10, 0.25)
        lip_noise_phase = intake - rand.uniform(0.10, 0.15)
        lip_noise_strength = rand.uniform(0.016, 0.065) if stride % 7 in (1, 4, 6) else 0.0
        for j in range(frames):
            p = j / frames
            blow = puff(p, 0.013, crest, expiration_end)
            draw = puff(p, expiration_end - 0.035, intake, breath_length)
            air = rand.random() * 2 - 1
            slow_noise += 0.008 * (air - slow_noise)
            flutter_hz += 0.0018 * (74 + 38 * blow + 260 * slow_noise - flutter_hz)
            glottis = (glottis + flutter_hz / RATE) % 1
            # Vocal folds barely engage on a strong out-breath. The intake has a
            # higher, unvoiced nasal rasp. Neither phase carries a sustained pitch.
            rough = (0.5 + 0.5 * math.cos(TAU * glottis)) ** 2.8
            pulse = 1.0 + flutter * blow * (rough - 0.29)
            chest = throat.sample(air)
            low = open_mouth.sample(air)
            high = exhale_air.sample(air)
            huff = (
                0.77 * chest
                + (1.45 - 0.48 * blow) * low
                + (0.49 + 0.70 * blow) * high
                + 0.32 * muzzle.sample(air)
            )
            inhale = (
                0.20 * low
                + 1.13 * nostril.sample(air)
                + 1.04 * inspiration.sample(air)
                + 0.42 * fine_air.sample(air)
            )
            # Subtle irregular pressure at the front of the muzzle avoids a flat noise gate.
            pressure = max(0.60, min(1.43, 1.0 + 2.4 * slow_noise))
            fleck = max(0.0, 1.0 - abs(p - lip_noise_phase) / 0.019)
            sound = effort * pressure * (huff * blow * pulse + intake_strength * draw * inhale)
            sound += lip_noise_strength * fleck * fleck * wet.sample(air)
            audio.append(math.tanh(sound * 1.25))
    # Headroom for the forest loops and Max's handoff. Both endpoints are silent.
    gain = 0.63 / max(abs(x) for x in audio)
    return array("h", (round(x * gain * 32767) for x in audio))


def main():
    pcm = render()
    if sys.byteorder != "little":
        pcm.byteswap()
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT), "wb") as output:
        output.setparams((1, 2, RATE, len(pcm), "NONE", "not compressed"))
        output.writeframes(pcm.tobytes())
    print(
        f"{OUTPUT.name} {len(pcm) / RATE:.3f} seconds, {STRIDES} running strides at {STRIDES_PER_SECOND} Hz"
    )


if __name__ == "__main__":
    main()
