#!/usr/bin/env python3
"""Genereaza sprite-urile lipsa (assets/sprites/) ca pixel art 16x16 scalat x4.

Doar stdlib (zlib + struct, PNG scris manual). Stil: flat, contur negru,
fundal transparent - consistent cu sprite-urile existente.
Ruleaza:  python3 tools/generate_sprites.py
"""
import os
import struct
import zlib

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")
SCALE = 4   # 16x16 -> 64x64


def write_png(name, grid, palette):
    h = len(grid)
    w = len(grid[0])
    W, H = w * SCALE, h * SCALE

    raw = b""
    for y in range(H):
        row = b"\x00"   # filter none
        for x in range(W):
            ch = grid[y // SCALE][x // SCALE]
            r, g, b, a = palette.get(ch, (0, 0, 0, 0))
            row += struct.pack("4B", r, g, b, a)
        raw += row

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", W, H, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")

    path = os.path.join(OUT_DIR, name)
    with open(path, "wb") as f:
        f.write(png)
    print(f"  {name}  ({W}x{H})")


# legenda comuna: '.' = transparent, 'k' = contur negru
def gen():
    # Antivirus: scut albastru cu cruce alba
    write_png("tower_antivirus.png", [
        "................",
        "....kkkkkkkk....",
        "...kBBBBBBBBk...",
        "..kBBBwwBBBBBk..",
        "..kBBBwwBBBbBk..",
        "..kBwwwwwwBbBk..",
        "..kBwwwwwwBbBk..",
        "..kBBBwwBBBbBk..",
        "..kBBBwwBBBbBk..",
        "..kBBBBBBBbBk...",
        "...kBBBBBBbBk...",
        "...kbBBBBbbk....",
        "....kbBBbbk.....",
        ".....kbbbk......",
        "......kkk.......",
        "................",
    ], {"B": (80, 140, 230, 255), "b": (50, 95, 170, 255),
        "w": (240, 245, 255, 255), "k": (10, 10, 20, 255)})

    # Miner: moneda aurie cu "B" (Bytecoin)
    write_png("tower_miner.png", [
        "................",
        ".....kkkkkk.....",
        "...kkGGGGGGkk...",
        "..kGGGGGGGGGGk..",
        ".kGGGwwwwGGGGGk.",
        ".kGGGwGGGwGGGdk.",
        "kGGGGwGGGwGGGGdk",
        "kGGGGwwwwGGGGGdk",
        "kGGGGwGGGwGGGGdk",
        "kGGGGwGGGwGGGddk",
        ".kGGGwwwwGGGGdk.",
        ".kGGGGGGGGGGddk.",
        "..kGGGGGGGGddk..",
        "...kkGGGGddkk...",
        ".....kkkkkk.....",
        "................",
    ], {"G": (235, 190, 70, 255), "d": (180, 135, 40, 255),
        "w": (255, 250, 230, 255), "k": (60, 40, 10, 255)})

    # Adware: fereastra de popup tipatoare cu "AD!"
    write_png("enemy_adware.png", [
        "................",
        ".kkkkkkkkkkkkkk.",
        ".kRRRRRRRRRRrXk.",
        ".kRRRRRRRRRRrXk.",
        ".kkkkkkkkkkkkkk.",
        ".kYYYYYYYYYYYYk.",
        ".kYkkYkkkYYkYYk.",
        ".kYkYkYkYYYkYYk.",
        ".kYkkkYkYYYkYYk.",
        ".kYkYkYkYYYYYYk.",
        ".kYkYkYkkkYkYYk.",
        ".kYYYYYYYYYYYYk.",
        ".kkkkkkkkkkkkkk.",
        "...kk......kk...",
        "...kk......kk...",
        "................",
    ], {"R": (230, 80, 60, 255), "r": (180, 50, 40, 255),
        "X": (255, 255, 255, 255), "Y": (250, 220, 90, 255),
        "k": (15, 15, 20, 255)})

    # Trojan: gandac blindat verde-inchis
    write_png("enemy_trojan.png", [
        "................",
        "...k..kkkk..k...",
        "....kkGGGGkk....",
        "...kGGgGGgGGk...",
        "..kGGGgGGgGGGk..",
        ".kGGGGgGGgGGGGk.",
        ".kGgggggggggGGk.",
        "kGGGGGgGGgGGGGGk",
        "kGGGGGgGGgGGGGGk",
        ".kGgggggggggGGk.",
        ".kGGGGgGGgGGGGk.",
        "..kGGGgGGgGGGk..",
        "...kGGgGGgGGk...",
        "....kkGGGGkk....",
        "...k..kkkk..k...",
        "................",
    ], {"G": (90, 130, 70, 255), "g": (50, 75, 40, 255),
        "k": (12, 18, 12, 255)})

    # Worm: vierme roz segmentat in S
    write_png("enemy_worm.png", [
        "................",
        "................",
        "....kkkk........",
        "...kPPPPk.......",
        "..kPwPwPPk......",
        "..kPPPPPPk......",
        "..kpPPPPpk......",
        "...kPPPPk.kkk...",
        "...kpPPPkkPPPk..",
        "....kPPPPPPPPk..",
        ".....kpppPPPpk..",
        "....kkkkkPPPk...",
        "...kPPPPPPPk....",
        "...kpppppppk....",
        "....kkkkkkk.....",
        "................",
    ], {"P": (235, 130, 150, 255), "p": (180, 85, 105, 255),
        "w": (20, 20, 30, 255), "k": (40, 15, 25, 255)})


if __name__ == "__main__":
    os.makedirs(OUT_DIR, exist_ok=True)
    print("Generez sprite-urile lipsa...")
    gen()
    print("Gata.")
