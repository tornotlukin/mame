# Jr. Pac-Man (`jrpacman`) — ROM & Game-State Map

Namco-style Z80 hardware. **Program ROM (`maincpu`) is ENCRYPTED** (static XOR table in
`init_jrpacman`, bits 0/2/7); gfx/proms/namco are plaintext. Decrypt via the project codec
(`pac-man-4ever/tools/jrpac_codec.py`) before disassembling. Cabinet is **ROT90**.

Sources: decrypted disasm (`pac-man-4ever/build/disasm/*.asm` via unidasm), driver
`src/mame/pacman/jrpacman.cpp` + `pacman_v.cpp`, and **live confirmation** in MAME
(headless Lua harness `pac-man-4ever/tools/play_inspect.lua`, 2026-06-21).

## ROM regions
| Region | Size | Files | Purpose |
|--------|------|-------|---------|
| maincpu | 0x10000 | 8d,8e,8h,8j,8k (5×0x2000 @ 0,2000,8000,a000,c000) | Z80 program (ENCRYPTED) |
| gfx1 | 0x4000 | 2c (tiles, 512×8x8 @0x0000), 2e (sprites, 128×16x16 @0x2000) | graphics, 2bpp planar |
| proms | 0x120 | 9e/9f (palette, 32 colors), 9p (colortable, 256) | color |
| namco | 0x200 | 7p (waveform), 5s (unused) | WSG sound |

## Hardware memory map
- 0x0000-0x3fff ROM, 0x8000-0xdfff ROM
- 0x4000-0x47ff videoram (tile codes + color attrs; jrpacman 36×54 scrolled tilemap)
- 0x4800-0x4fef work RAM (game state)
- 0x4ff0-0x4fff spriteram (8 slots × 2 bytes: image/flip, color)
- 0x5000 r=P1 (b0 UP,b1 LEFT,b2 RIGHT,b3 DOWN active-low; b5 COIN1,b6 COIN2,b7 COIN3) / w=latch1
- 0x5040 r=P2 (joy cocktail; b5 START1,b6 START2) / w 5040-505f=WSG sound
- 0x5060-0x506f w=spriteram2 (8 × X/Y coord pairs)
- 0x5070-0x5077 latch2 (70 palettebank,71 colortablebank,73 bgpriority,74 charbank,75 spritebank)
- 0x5080 r=DSW1 (b0-1 coinage,b2-3 lives,b4-5 bonus,b6 difficulty,b7 unused) / w=scroll
- 0x50c0 watchdog. IM2, I=0x3F, interrupt vector set via `OUT (0),a`.

## Game-state RAM (✅ = live-confirmed)
| Addr | Field | Enc | Evidence |
|------|-------|-----|----------|
| **0x4D00 / 0x4D01** | Pac world pos X / Y ✅ | 2×byte | `1433: ld a,($4D00)`; live: moved 5C,80→8C,7C in play |
| **0x4D02..0x4D09** | Ghosts 1-4 world pos (X,Y pairs) ✅ | 8 bytes | `1442..146f`; live: 4 ghosts clustered ~74,80 |
| **0x4D30** | Pac current direction (0=U,1=R,2=D,3=L) ✅ | byte | `194d: ld ($4D30),a`; live: 2,3 valid |
| **0x4D3C** | Pac desired/next direction ✅ | byte | `1ace: ld ($4D3C),a` |
| 0x4D1C/1D, 0x4D26/27 | current / desired velocity-delta word | 2×word | `1893`, `18df` |
| 0x4D14-0x4D2F | per-actor persistent delta block | — | init from table @0x25C1 |
| 0x4D31 / 0x4D33 | Pac tile pos (dot/collision) | 2 bytes | `1759`, `1749` |
| **0x4C02 / 0x4C03** | Pac sprite color / tile (frame buf) ✅ | 2 bytes | `2541/2559`; DMA'd to spriteram |
| 0x4C04-0x4C0B | ghost sprite color/tile pairs | 8 bytes | frightened rewrite `1a9d` |
| 0x4C12/13 .. 0x4C1B | actor screen Y/X (frame buf) | — | `143f/1437`, `1442+` |
| **0x4E0E** | dots eaten (counts up, level clears at 0xF4=244) ✅ | byte | `9a8b: cp $F4; inc(hl)`; live 0→2 |
| **0x4E13** | level / round number (0-based) ✅ | byte | `0a90: inc(hl)`; live=0 on first board |
| **0x4E14 / 0x4E15** | P1 / P2 lives ✅ | byte | `0696/0699 init`, `2b44 inc`, `06a8 dec`; live: =3 at start |
| **0x4E80-0x4E82** | P1 score, 3-byte packed BCD LE ✅ | 3 bytes | `2b0e: ld hl,$4E80`; live 0→0x40 |
| 0x4E84-0x4E86 | P2 score (BCD) | 3 bytes | `2b13` |
| 0x4E88-0x4E8A | high score (BCD) | 3 bytes | `2a8c` |
| 0x4E09 | current player index (0=P1,1=P2) | byte | `2b0b` |
| 0x4E00 | game-state-machine index | byte | `0195` gates gameplay calls |
| 0x4E6B/6D/6E/6F/71/73/75 | parsed DSW values (coinage, lives, bonus, flags) | — | DSW parser @0x26D0 |
| 0x4CC0-0x4CFF | dot/maze display-update buffer (fills 0xFF) | 64 B | `2382 fill`; ptrs 0x4C80/0x4C82 |
| 0x4FF0-0x4FFF | spriteram: 8 slots image/color ✅ | 16 B | pac img@0x4FF2 (live 80/90/98 chomp); obj@0x4FFC; **slot7@0x4FFE spare (live=00)** |

## Key routines
| Addr | Routine |
|------|---------|
| 0x008D | **per-frame VBLANK ISR** entry (watchdog, IRQ ack, → 0x9BE0; sprite DMA & update chain) |
| 0x0176 / 0x0181 | **sprite DMA**: 0x4C22→0x4FF2 (12B image/color) ; 0x4C32→0x5062 (12B coords) — the logical→hardware chokepoint |
| 0x238D | foreground main loop (rst $20 state-machine dispatch) |
| 0x18AB / 0x1874 | input → direction (reads 0x5000 P1 / 0x5040 P2; cocktail flag selects) |
| 0x1429 | build screen coords from world pos (0x4D00→0x4C12) |
| 0x2A5A | **add-to-score** (BCD `daa`; value table @0x2B17; bonus-life + hi-score check) |
| 0x06A8 | death: decrement lives, redraw icons |
| 0x26D0 | DSW1 parser (boot); 0x95FD = per-frame difficulty-bit helper |
| 0x0A90 | level increment on maze clear |

## Notes for 4-player mod (pac-man-4ever)
- **8 hardware sprite slots**; game uses 6 (Pac+4 ghosts+bonus). **Slot 7 (img 0x4FFE / coord 0x506E) is spare** (live=00). 4 Pacs + 4 ghosts = 8 fits exactly (no bonus sprite).
- **Sprite DMA @0x0176/0x0181** is the single place to widen to push more/own sprite slots.
- **Per-player input already abstracted** (cocktail flag selects 0x5000 vs 0x5040 in 0x18AB/0x1874) — extend with P3/P4 sources + per-player dir/delta working vars (mirror 0x4D30/0x4D3C/0x4D1C/0x4D26).
- Candidate spare work-RAM for extra-player state: 0x4D0A-0x4D13 (gap), 0x4E2x-0x4E5x band (untraced) — verify before use.
- **POST RAM test runs first ~2.5s** (fills RAM with a +3 ramp); ignore game-state reads until it finishes / a game is started.
- **Scroll / sprite coords are WORLD-relative, not screen-relative.** Sprite Y in the frame
  buffer (0x4C12) = world Y (0x4D01) + 6, constant, *independent of the scroll register*
  (0x5080 write, observed 0x64-0x9A). In stock the maze tilemap scrolls (set_scrolly) to keep
  the world-positioned actors on the 224px screen. Consequence (used by pac-man-4ever's
  widescreen): force tilemap scroll to 0 and widen the visible raster to the full 36×54 maze
  (432px) and all sprites stay aligned — no per-sprite scroll compensation required.

## Tooling (in pac-man-4ever/tools)
- `jrpac_codec.py` — encrypt/decrypt program ROM (required for any code edit).
- `gfx_jrpac.py` + `sprite_extract.py`/`sprite_import.py` — gfx ↔ PNG (lossless, ROT90-upright).
- `screen_render.py` — render a videoram snapshot → PNG (offline screenshots).
- `mame_dbg.py` — TCP client for the remote debugger (`-debugger remote -debugger_port 12345`); note: this build can crash on repeated `gtime` over the socket — the `-debugscript` and Lua paths are more stable.
- `play_inspect.lua` — headless coin-up/start/drive + RAM read (reusable test harness).
