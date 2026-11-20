#!/usr/bin/env python3
"""Reproducible original PCM effects. Default writes; --check only verifies."""
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import random
import struct
import wave

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = ROOT / "assets/embervale/audio-pilot"
RATE = 48000
# Name, variants, seconds, partial frequencies, noise, decay.
CUES = [
    ("wood", 2, .22, (110, 260, 640), .48, 25),
    ("stone", 2, .25, (540, 1190, 2380), .25, 19),
    ("swing", 1, .18, (180,), 1.0, 12),
    ("heavy-swing", 1, .28, (80, 125), .9, 9),
    ("hit", 2, .18, (75, 140, 330), .6, 32),
    ("block", 1, .28, (680, 1080, 1650), .18, 15),
    ("miss", 1, .12, (290,), .7, 24),
    ("gather", 1, .16, (740, 1110), .03, 16),
    ("craft", 1, .25, (430, 645, 860), .08, 12),
    ("equip", 1, .14, (430, 1230), .22, 24),
    ("inventory", 1, .075, (260, 520), .18, 45),
    ("eat", 1, .18, (210, 340), .48, 22),
]

def synthesize(index, variant):
    name, _, seconds, frequencies, noise_amount, decay = CUES[index]
    rng = random.Random(9721 + index * 137 + variant * 19)
    samples, smooth = [], 0.0
    shift = 1 + (variant - .5) * .045
    count = round(seconds * RATE)
    for i in range(count):
        t = i / RATE
        noise = rng.uniform(-1, 1)
        smooth += .18 * (noise - smooth)
        tonal = sum(math.sin(2 * math.pi * f * shift * t) / (n + 1)
                    for n, f in enumerate(frequencies)) / len(frequencies)
        if "swing" in name or name == "miss":
            envelope = math.sin(math.pi * i / (count - 1)) ** 2
            value = (noise - smooth) * noise_amount + tonal * .12
        else:
            envelope = min(1, t / .004) * math.exp(-decay * t)
            value = tonal * (1 - noise_amount * .5) + smooth * noise_amount
        release = min(1, (count - 1 - i) / (RATE * .014))
        samples.append(value * envelope * release)
    peak = max(abs(v) for v in samples)
    # Deliberate headroom: six voices at 0.45 gain sum to <= 0.756 peak.
    pcm = [round(value / peak * .28 * 32767) for value in samples]
    stream = io.BytesIO()
    with wave.open(stream, "wb") as wav:
        wav.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        wav.writeframes(struct.pack("<" + "h" * len(pcm), *pcm))
    return stream.getvalue(), pcm

def build(check=False):
    manifest = {
        "version": 1, "generator": "tools/audio/build_audio_pilot.py",
        "format": {"sample_rate": RATE, "channels": 1, "bits": 16, "encoding": "PCM little endian"},
        "provenance": "Original mathematical synthesis created for Adzffs/GameEngine with AI assistance; no sampled or third-party audio input. No separate licence grant; repository distribution policy applies.",
        "clips": [],
    }
    for index, (name, variants, *_) in enumerate(CUES):
        for variant in range(variants):
            data, pcm = synthesize(index, variant)
            filename = f"{name}-{variant + 1}.wav"
            record = {"file": filename, "sha256": hashlib.sha256(data).hexdigest(),
                      "frames": len(pcm), "seconds": len(pcm) / RATE,
                      "peak": max(abs(v) for v in pcm) / 32768,
                      "rms": math.sqrt(sum(v * v for v in pcm) / len(pcm)) / 32768}
            assert pcm[0] == pcm[-1] == 0 and record["peak"] <= .281
            assert .02 < record["seconds"] <= .5 and record["rms"] > .001
            manifest["clips"].append(record)
            path = DIRECTORY / filename
            if check:
                if not path.is_file() or path.read_bytes() != data:
                    raise ValueError(f"Missing/stale sound: {path.relative_to(ROOT)}")
                with wave.open(str(path), "rb") as wav:
                    assert (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) == (1, 2, RATE)
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
    encoded = json.dumps(manifest, indent=2) + "\n"
    path = DIRECTORY / "manifest.json"
    if check:
        if not path.is_file() or path.read_text() != encoded:
            raise ValueError("Missing/stale audio manifest")
    else:
        path.write_text(encoded)
    print(f"{'Verified' if check else 'Wrote'} {len(manifest['clips'])} original PCM effects")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    build(parser.parse_args().check)
