#!/usr/bin/env python3
"""Genereaza procedural toate fisierele audio ale jocului (assets/audio/).

Doar stdlib (wave, math, random) - fara dependinte, fara probleme de licenta.
Ruleaza din radacina repo:  python3 tools/generate_audio.py
"""
import math
import os
import random
import struct
import wave

SR = 22050
OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "audio")

random.seed(0xC0DEBABE)


# ---------- helpers ----------

def silence(dur):
    return [0.0] * int(SR * dur)

def sine(freq, dur, vol=1.0):
    return [vol * math.sin(2 * math.pi * freq * t / SR) for t in range(int(SR * dur))]

def square(freq, dur, vol=1.0):
    out = []
    for t in range(int(SR * dur)):
        phase = (freq * t / SR) % 1.0
        out.append(vol if phase < 0.5 else -vol)
    return out

def saw(freq, dur, vol=1.0):
    out = []
    for t in range(int(SR * dur)):
        phase = (freq * t / SR) % 1.0
        out.append(vol * (2.0 * phase - 1.0))
    return out

def noise(dur, vol=1.0):
    return [vol * (random.random() * 2 - 1) for _ in range(int(SR * dur))]

def sweep(f0, f1, dur, vol=1.0, shape="sine"):
    """Frecventa gliseaza liniar f0 -> f1."""
    out = []
    n = int(SR * dur)
    phase = 0.0
    for t in range(n):
        f = f0 + (f1 - f0) * t / n
        phase += f / SR
        if shape == "sine":
            out.append(vol * math.sin(2 * math.pi * phase))
        else:
            out.append(vol if (phase % 1.0) < 0.5 else -vol)
    return out

def env(samples, attack=0.005, release=0.05):
    """Atac si release liniare ca sa nu pocneasca capetele."""
    n = len(samples)
    a = max(1, int(SR * attack))
    r = max(1, int(SR * release))
    out = list(samples)
    for i in range(min(a, n)):
        out[i] *= i / a
    for i in range(min(r, n)):
        out[n - 1 - i] *= i / r
    return out

def decay(samples, half_life=0.06):
    """Decadere exponentiala (percutie)."""
    k = math.log(2) / (half_life * SR)
    return [s * math.exp(-k * i) for i, s in enumerate(samples)]

def lowpass(samples, alpha=0.15):
    out = []
    y = 0.0
    for x in samples:
        y += alpha * (x - y)
        out.append(y)
    return out

def mix(*tracks):
    n = max(len(t) for t in tracks)
    out = [0.0] * n
    for t in tracks:
        for i, s in enumerate(t):
            out[i] += s
    return out

def concat(*tracks):
    out = []
    for t in tracks:
        out.extend(t)
    return out

def normalize(samples, peak=0.8):
    m = max(1e-9, max(abs(s) for s in samples))
    return [s * peak / m for s in samples]

def write_wav(name, samples, peak=0.8):
    samples = normalize(samples, peak)
    path = os.path.join(OUT_DIR, name)
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = b"".join(
            struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples)
        w.writeframes(frames)
    print(f"  {name}  ({len(samples)/SR:.2f}s)")


# ---------- SFX ----------

def gen_sfx():
    # Antivirus: laser descendent
    write_wav("shoot_laser.wav", env(decay(sweep(900, 280, 0.14), 0.05)))

    # Adblocker: tick scurt
    write_wav("shoot_tick.wav", env(decay(square(1250, 0.05, 0.6), 0.015)), peak=0.5)

    # Impact pe inamic
    write_wav("hit.wav",
              env(mix(decay(lowpass(noise(0.09), 0.25), 0.03),
                      decay(sine(190, 0.09), 0.04))), peak=0.55)

    # Kill: pop descendent
    write_wav("kill.wav", env(decay(sweep(420, 90, 0.18), 0.07)))

    # Inamic a ajuns la baza: alarma в doua tonuri
    leak = concat(square(240, 0.1, 0.7), square(170, 0.1, 0.7),
                  square(240, 0.1, 0.7), square(170, 0.16, 0.7))
    write_wav("leak.wav", env(lowpass(leak, 0.4)))

    # Start de val: riser
    write_wav("wave_start.wav", env(lowpass(sweep(140, 620, 0.5, shape="saw"), 0.3),
                                    attack=0.05, release=0.1))

    # Final de val: arpegiu ascendent (A minor: A C E A)
    chime = concat(*(decay(sine(f, 0.16), 0.1)
                     for f in (440.0, 523.25, 659.25, 880.0)))
    write_wav("wave_end.wav", env(chime))

    # Cumparare in shop
    write_wav("buy.wav", env(concat(decay(sine(880, 0.07), 0.04),
                                    decay(sine(1320, 0.12), 0.06))))

    # Eroare / bani insuficienti
    write_wav("error.wav", env(lowpass(square(115, 0.22, 0.8), 0.35)))

    # Plasare turn: thud
    write_wav("place.wav", env(mix(decay(sine(120, 0.16), 0.05),
                                   decay(lowpass(noise(0.05), 0.2), 0.02))))

    # Vanzare turn: monede
    write_wav("sell.wav", env(concat(decay(sine(1700, 0.08), 0.04),
                                     decay(sine(1400, 0.12), 0.05))))

    # Moneda de income ajunsa la contor: ding scurt si luminos (fundamentala
    # + cvinta); pitch-ul urca din cod la fiecare moneda care aterizeaza.
    write_wav("coin.wav", env(decay(mix(sine(1568, 0.1),
                                        sine(2349, 0.1, 0.4)), 0.045)), peak=0.5)

    # Aplicare token: power-up cu vibrato
    n = int(SR * 0.35)
    tok = []
    phase = 0.0
    for t in range(n):
        f = 300 + 600 * t / n + 30 * math.sin(2 * math.pi * 9 * t / SR)
        phase += f / SR
        tok.append(0.7 if (phase % 1.0) < 0.5 else -0.7)
    write_wav("token.wav", env(lowpass(tok, 0.35), release=0.08))

    # Jingles (game over / victory)
    win = concat(*(decay(mix(sine(f, 0.3), sine(f * 2, 0.3, 0.3)), 0.18)
                   for f in (440.0, 554.37, 659.25, 880.0)))
    write_wav("jingle_win.wav", env(win, release=0.3))

    lose = concat(*(decay(mix(saw(f, 0.4, 0.5), sine(f / 2, 0.4, 0.5)), 0.25)
                    for f in (440.0, 415.30, 349.23, 220.0)))
    write_wav("jingle_lose.wav", env(lowpass(lose, 0.25), release=0.4))


# ---------- muzica: dark synthwave loop ----------

NOTE = {"A1": 55.0, "E2": 82.41, "F2": 87.31, "G2": 98.0, "A2": 110.0,
        "C3": 130.81, "E3": 164.81, "F3": 174.61, "G3": 196.0, "A3": 220.0}

def gen_music():
    bpm = 100
    beat = 60.0 / bpm                 # 0.6s
    bar = 4 * beat                    # 2.4s
    progression = ["A2", "F2", "G2", "E2"] * 4   # 16 bars
    total = len(progression) * bar
    n_total = int(SR * total)

    track = [0.0] * n_total

    def add(samples, start_sec, vol=1.0):
        start = int(start_sec * SR)
        for i, s in enumerate(samples):
            j = start + i
            if 0 <= j < n_total:
                track[j] += s * vol

    for bar_i, root_name in enumerate(progression):
        t0 = bar_i * bar
        root = NOTE[root_name]

        # Bass: saw pe optimi, gated
        for eighth in range(8):
            add(env(decay(lowpass(saw(root, beat * 0.40, 0.9), 0.5), 0.25),
                    attack=0.004, release=0.03),
                t0 + eighth * beat / 2, vol=0.50)

        # Kick pe fiecare beat: drop de sinus
        for b in range(4):
            add(decay(sweep(120, 38, 0.22), 0.05), t0 + b * beat, vol=0.85)

        # Snare (noise) pe 2 si 4
        for b in (1, 3):
            add(env(decay(noise(0.14), 0.035)), t0 + b * beat, vol=0.32)

        # Hat: 16ths foarte discrete
        for s16 in range(16):
            add(env(decay(noise(0.03), 0.008)), t0 + s16 * beat / 4, vol=0.06)

        # Pad: root + quinta + octava, detunat, atac lent, lowpass greu
        pad = mix(lowpass(saw(root * 2, bar, 0.5), 0.04),
                  lowpass(saw(root * 2 * 1.005, bar, 0.5), 0.04),
                  lowpass(saw(root * 3, bar, 0.35), 0.04))
        add(env(pad, attack=0.6, release=0.5), t0, vol=0.22)

        # Arp: 16ths square (root/quinta/octava sus), abia audibil dar da miscare
        arp_steps = [2, 3, 4, 3]
        for s16 in range(16):
            f = root * arp_steps[s16 % 4]
            add(env(decay(square(f, beat / 4 * 0.8, 0.4), 0.05),
                    attack=0.003, release=0.02),
                t0 + s16 * beat / 4, vol=0.10)

    # Crossfade cap-coada pentru loop fara click (ultimele 60ms peste primele).
    xf = int(0.06 * SR)
    for i in range(xf):
        a = i / xf
        track[i] = track[i] * a + track[n_total - xf + i] * (1 - a)
    track = track[: n_total - xf]

    write_wav("music_loop.wav", track, peak=0.62)


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    print("Generez SFX...")
    gen_sfx()
    print("Generez muzica (dureaza putin)...")
    gen_music()
    print("Gata.")
