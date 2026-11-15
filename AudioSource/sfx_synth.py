"""Retake - procedural sound effects (numpy only, no samples).
Usage: python sfx_synth.py [out_dir]   -> writes 13 mono 44.1 kHz 16-bit WAVs named like the audio slots."""
import numpy as np, wave, sys, os
SR = 44100
rng = np.random.default_rng(11)

def T(d): return np.arange(int(d * SR)) / SR
def lp(x, fc):
    X = np.fft.rfft(x); f = np.fft.rfftfreq(len(x), 1 / SR)
    return np.fft.irfft(X / np.sqrt(1 + (f / fc) ** 4), len(x))
def hp(x, fc):
    X = np.fft.rfft(x); f = np.fft.rfftfreq(len(x), 1 / SR)
    return np.fft.irfft(X * (f / fc) ** 2 / np.sqrt(1 + (f / fc) ** 4), len(x))
def bp(x, lo, hi): return hp(lp(x, hi), lo)
def noise(d): return rng.standard_normal(int(d * SR))
def att(t, a=0.002): return np.minimum(1, t / a)
def sweep(t, f0, f1, k): return np.sin(2 * np.pi * np.cumsum(f1 + (f0 - f1) * np.exp(-t * k)) / SR)
def mix(d, *parts):
    out = np.zeros(int(d * SR))
    for start, sig, g in parts:
        i = int(start * SR); n = min(len(sig), len(out) - i)
        out[i:i + n] += sig[:n] * g
    return out
def click(d, lo, hi, decay):
    t = T(d); return bp(noise(d), lo, hi) * np.exp(-t * decay) * att(t, 0.0005)
def thud(d, f0, f1, decay, k=30):
    t = T(d); return sweep(t, f0, f1, k) * np.exp(-t * decay) * att(t)
def tone(d, f, decay, harm=(1, 0.3, 0.1)):
    t = T(d); s = sum(a * np.sin(2 * np.pi * f * (i + 1) * t) for i, a in enumerate(harm))
    return s * np.exp(-t * decay) * att(t)

def clapperboard():   # two wooden sticks snapping shut
    return mix(0.35, (0, click(0.12, 900, 6000, 55), 1.0), (0, thud(0.12, 420, 180, 40), 0.7),
               (0.012, click(0.08, 1500, 8000, 90), 0.5), (0.02, bp(noise(0.3), 300, 2500) * np.exp(-T(0.3) * 18), 0.12))
def plate_on():       # heavy metal plate sinks + rising confirm blip
    return mix(0.4, (0, thud(0.2, 160, 70, 22), 1.0), (0, click(0.05, 2000, 7000, 120), 0.35),
               (0.05, tone(0.3, 660, 12), 0.25), (0.11, tone(0.28, 990, 12), 0.25))
def plate_off():      # plate springs back + falling blip
    return mix(0.35, (0, thud(0.15, 90, 150, 28, k=-8), 0.7), (0, click(0.04, 1500, 6000, 140), 0.3),
               (0.04, tone(0.25, 880, 14), 0.2), (0.10, tone(0.25, 590, 14), 0.2))
def lever():          # ratchet clicks then a clunk
    parts = [(0.03 * i, click(0.03, 1800, 7000, 220), 0.35 + 0.05 * i) for i in range(4)]
    return mix(0.5, *parts, (0.13, thud(0.25, 240, 85, 20), 1.0), (0.13, click(0.1, 500, 3500, 45), 0.6),
               (0.14, tone(0.3, 1240, 16, (1, 0.5, 0.3, 0.2)), 0.12))
def gate(opening):    # motor/chain rumble sliding up or down, ends in a stop clunk
    d = 0.9; t = T(d); f = (60 + 50 * t / d) if opening else (110 - 50 * t / d)
    ph = 2 * np.pi * np.cumsum(f) / SR
    motor = (np.sign(np.sin(ph)) * 0.3 + np.sin(ph)) * (0.6 + 0.4 * np.sin(2 * np.pi * 23 * t))
    motor = lp(motor, 700) * att(t, 0.05) * np.minimum(1, (d - t) / 0.08)
    rattle = bp(noise(d), 1200, 5000) * (0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 17 * t))) * att(t, 0.05) * np.minimum(1, (d - t) / 0.08)
    return mix(1.2, (0, motor, 0.55), (0, rattle, 0.10), (d - 0.03, thud(0.3, 200 if opening else 130, 60, 18), 1.0),
               (d - 0.03, click(0.12, 600, 4000, 40), 0.5))
def explosive_arm():  # match strike + fizzing fuse with two warning beeps
    d = 0.9; t = T(d)
    fizz = hp(noise(d), 3500) * (0.5 + 0.5 * rng.random(len(t))) * att(t, 0.03) * np.minimum(1, (d - t) / 0.1)
    fizz *= 0.7 + 0.3 * np.sin(2 * np.pi * 31 * t)
    return mix(d, (0, click(0.1, 2500, 9000, 35), 0.8), (0.03, fizz, 0.35),
               (0.10, tone(0.12, 1320, 30, (1, 0.2)), 0.35), (0.40, tone(0.12, 1320, 30, (1, 0.2)), 0.35))
def explosion():      # crack, sub boom, long rumble, falling debris
    d = 2.4; t = T(d)
    boom = sweep(t, 110, 32, 5) * np.exp(-t * 2.6) * att(t, 0.003)
    body = lp(noise(d), 900) * np.exp(-t * 3.2) * att(t, 0.004)
    rumble = lp(noise(d), 180) * np.exp(-t * 1.5) * att(t, 0.05) * 3
    crack = bp(noise(0.2), 800, 9000) * np.exp(-T(0.2) * 28)
    debris = np.zeros(len(t))
    for _ in range(26):
        s = 0.25 + rng.random() * 1.5; c = click(0.05, 1500 + rng.random() * 3000, 9000, 90 + rng.random() * 80)
        i = int(s * SR); debris[i:i + len(c)] += c * (0.5 * np.exp(-s * 1.4))
    out = mix(d, (0, crack, 0.9), (0, boom, 1.3), (0, body, 0.9), (0, rumble, 0.5), (0, debris, 0.35))
    return np.tanh(out * 1.6)    # a bit of saturation so it hits
def guard_alert():    # two rising brass-like "hey!" stabs
    def stab(f, d):
        t = T(d); vib = 1 + 0.01 * np.sin(2 * np.pi * 6 * t)
        s = sum(np.sin(2 * np.pi * f * k * vib * t) / k for k in range(1, 9))
        return lp(s, 2600) * att(t, 0.012) * np.minimum(1, (d - t) / 0.05)
    return mix(0.6, (0, stab(392, 0.16), 0.5), (0.2, stab(523, 0.34), 0.55))
def guard_catch():    # grab thump + whistle blast
    d = 0.6; t = T(d); f = 2350 + 60 * np.sign(np.sin(2 * np.pi * 38 * t))
    whistle = np.sin(2 * np.pi * np.cumsum(f) / SR) * att(t, 0.01) * np.minimum(1, (d - t) / 0.06)
    whistle += 0.3 * bp(noise(d), 2000, 5000) * att(t, 0.01) * np.minimum(1, (d - t) / 0.06)
    return mix(0.8, (0, thud(0.25, 180, 60, 16), 1.0), (0, click(0.1, 300, 2500, 40), 0.5), (0.12, whistle, 0.3))
def crate_push():     # wood scraping on concrete
    d = 0.7; t = T(d)
    scrape = bp(noise(d), 250, 2200) * (0.55 + 0.45 * lp(np.abs(noise(d)), 40) * 6).clip(0, 1.5)
    scrape *= att(t, 0.04) * np.minimum(1, (d - t) / 0.2)
    return mix(d, (0, thud(0.15, 140, 70, 30), 0.6), (0, scrape, 0.7), (0, lp(noise(d), 120) * att(t, 0.04) * np.minimum(1, (d - t) / 0.2), 1.2))
def goal():           # bright 70s electric-piano arpeggio (C E G C)
    def ep(f, d):
        t = T(d); return (np.sin(2 * np.pi * f * t + 1.5 * np.exp(-t * 7) * np.sin(2 * np.pi * f * t)) +
                          0.2 * np.sin(2 * np.pi * f * 4 * t) * np.exp(-t * 12)) * np.exp(-t * 3.2) * att(t, 0.003)
    notes = [523.25, 659.25, 783.99, 1046.5]
    return mix(1.6, *[(0.085 * i, ep(f, 1.2), 0.4) for i, f in enumerate(notes)], (0.26, ep(1318.5, 1.2), 0.25))
def ui_click():       # soft typewriter key
    return mix(0.12, (0, click(0.05, 1200, 7000, 160), 0.8), (0, thud(0.06, 320, 200, 70), 0.5))

SOUNDS = {"SfxClapperboard": clapperboard, "SfxPlateOn": plate_on, "SfxPlateOff": plate_off, "SfxLever": lever,
          "SfxGateOpen": lambda: gate(True), "SfxGateClose": lambda: gate(False), "SfxExplosiveArm": explosive_arm,
          "SfxExplosion": explosion, "SfxGuardAlert": guard_alert, "SfxGuardCatch": guard_catch,
          "SfxCratePush": crate_push, "SfxGoal": goal, "SfxUIClick": ui_click}
PEAK = {"SfxGuardAlert": 0.5, "SfxExplosion": 0.97, "SfxUIClick": 0.5, "SfxCratePush": 0.6, "SfxPlateOff": 0.6, "SfxExplosiveArm": 0.7}

if __name__ == "__main__":
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    for name, fn in SOUNDS.items():
        x = fn(); n = len(x); fade = int(0.01 * SR)
        x[-fade:] *= np.linspace(1, 0, fade)                     # no click at the tail
        x = x / (np.max(np.abs(x)) + 1e-9) * PEAK.get(name, 0.8)
        with wave.open(os.path.join(out_dir, name + ".wav"), "wb") as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
            w.writeframes((x * 32767).astype("<i2").tobytes())
        print(f"{name}.wav  {n / SR:.2f} s")
