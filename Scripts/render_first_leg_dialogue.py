"""Render the six provisional voices locally using Flite 2.2 and ffmpeg."""
import os
from pathlib import Path
import subprocess
import tempfile

dest = Path(__file__).resolve().parents[1] / 'Art/Source/FirstLeg/Voices'
dest.mkdir(parents=True, exist_ok=True)
flite = os.environ.get('FLITE_BIN', 'flite')
lines = [
    ('BanditDemand', "Hands up! Don't move.", 105, 1.05, 6000),
    ('BanditGold', 'The bag. Put it down. Slowly.', 99, 1.1, 6000),
    ('BanditStay', 'Stay right there.', 95, 1.04, 6000),
    ('HermitWarning', 'Stay away from my door.', 80, 1.18, 1600),
    ('HermitShout', 'I warned you!', 94, .91, 5200),
    ('HermitPit', 'Get me out of here!', 79, 1.05, 2500),
]
with tempfile.TemporaryDirectory() as folder:
    for name, words, pitch, duration, cutoff in lines:
        raw = Path(folder) / 'line.wav'
        subprocess.run([flite, '-voice', 'kal16', '--setf', f'int_f0_target_mean={pitch}',
                        '--setf', f'duration_stretch={duration}', '-t', words, '-o', str(raw)], check=True)
        subprocess.run(['ffmpeg','-loglevel','error','-y','-i',str(raw),'-af',
                        f'highpass=f=85,lowpass=f={cutoff},loudnorm=I=-18:TP=-2:LRA=8',
                        '-ar','24000','-ac','1','-c:a','pcm_s16le',str(dest / (name+'.wav'))],check=True)
